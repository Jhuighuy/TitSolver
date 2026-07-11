/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { spawn, type ChildProcessWithoutNullStreams } from "node:child_process";
import { constants as osConstants } from "node:os";
import path from "node:path";

import pidusage from "pidusage";

import {
  openStorage as nativeOpenStorage,
  type Storage as NativeStorage,
} from "~/bindings";
import type { Installation } from "~/main/installation";
import { broadcastIpcEvent } from "~/main/ipc";
import { AsyncLruCache } from "~/main/lru-cache";
import type { SolverEvent } from "~/shared/solver";
import type { Frame } from "~/shared/storage";
import { assert } from "~/shared/utils";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export interface SessionOptions {
  workDir: string;
  storagePath: string;
  solverPath: string;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
 * A solver manager.
 */
class SolverManager {
  private child?: ChildProcessWithoutNullStreams;
  private listener?: (event: SolverEvent) => void;
  private telemetryTimer?: NodeJS.Timeout;

  /**
   * Create a solver manager.
   */
  public constructor(private readonly install: Installation) {}

  /**
   * Run the solver.
   */
  public run(workDir: string) {
    assert(this.child === undefined);
    const child = spawn(this.install.solverPath, [], { cwd: workDir });

    child.stdout.setEncoding("utf8");
    child.stdout.on("data", (data: string) => {
      this.listener?.({ kind: "stdout", data });
    });

    child.stderr.setEncoding("utf8");
    child.stderr.on("data", (data: string) => {
      this.listener?.({ kind: "stderr", data });
    });

    child.on("exit", (code, signal) => {
      this.child = undefined;
      this.stopTelemetry();
      this.listener?.({
        kind: "exit",
        code: code ?? 0,
        signal: signal === null ? 0 : osConstants.signals[signal],
      });
    });

    child.on("error", (error) => {
      this.child = undefined;
      this.stopTelemetry();
      this.listener?.({ kind: "stderr", data: String(error) });
      this.listener?.({ kind: "exit", code: 1, signal: 0 });
    });

    this.child = child;
    this.startTelemetry();
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  // Start sampling the resource usage of the solver process.
  private startTelemetry() {
    this.stopTelemetry();

    const childPID = this.child?.pid;
    if (childPID === undefined) return;

    const sample = async () => {
      if (this.child?.pid !== childPID) return;
      try {
        const usage = await pidusage(childPID);
        if (this.child?.pid !== childPID) return;

        this.listener?.({
          kind: "sample",
          timestamp: Date.now(),
          cpuPercent: usage.cpu,
          memoryBytes: usage.memory,
        });
      } catch {
        // Ignore telemetry errors caused by process shutdown races.
      }
    };

    void sample();
    this.telemetryTimer = setInterval(() => {
      void sample();
    }, TELEMETRY_INTERVAL_MS);
  }

  // Stop sampling and drop the sampler's internal process cache.
  private stopTelemetry() {
    if (this.telemetryTimer !== undefined) {
      clearInterval(this.telemetryTimer);
      this.telemetryTimer = undefined;
    }
    pidusage.clear();
  }

  /**
   * Stop the solver.
   */
  public stop() {
    if (this.child === undefined) return;
    this.child.kill("SIGTERM");
  }

  /**
   * Check if the solver is running.
   */
  public isRunning() {
    return this.child !== undefined;
  }

  /**
   * Set the event callback.
   */
  public setEventCallback(listener?: (event: SolverEvent) => void) {
    this.listener = listener;
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
 * A session manager.
 */
export class SessionManager {
  private readonly solver: SolverManager;
  private storage?: NativeStorage;
  private storagePollTimer?: NodeJS.Timeout;
  private storageDataVersion?: number;
  private isPollingStorage = false;
  private readonly frameCache = new AsyncLruCache<number, Frame>(
    FRAME_CACHE_CAPACITY,
  );
  private frameCount?: number;

  /**
   * Create a session manager.
   */
  public constructor(
    install: Installation,
    private readonly workDir: string,
  ) {
    this.solver = new SolverManager(install);
    this.solver.setEventCallback((event) => {
      broadcastIpcEvent("session", "solverEvent", event);
    });
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /**
   * Start the session.
   */
  public async start() {
    // Storage path is hardcoded at the moment.
    const storagePath = path.join(this.workDir, "particles.ttdb");
    this.storage = await nativeOpenStorage(storagePath);

    // Watch for changes made by other processes (e.g. a running solver).
    // SQLite has no cross-process change notifications; polling the
    // `data_version` pragma is the cheapest reliable signal.
    this.storageDataVersion = undefined;
    this.storagePollTimer = setInterval(() => {
      void this.pollStorageChanges();
    }, STORAGE_POLL_INTERVAL_MS);
  }

  /**
   * Stop the session.
   */
  public stop() {
    if (this.storagePollTimer !== undefined) {
      clearInterval(this.storagePollTimer);
      this.storagePollTimer = undefined;
    }
    this.frameCache.clear();
    this.frameCount = undefined;
    this.solver.stop();
    this.storage = undefined;
  }

  // Broadcast a storage change event when the data version moves.
  private async pollStorageChanges() {
    if (this.isPollingStorage || this.storage === undefined) return;
    this.isPollingStorage = true;
    try {
      const dataVersion = await this.storage.dataVersion();
      if (
        this.storageDataVersion !== undefined &&
        dataVersion !== this.storageDataVersion
      ) {
        // Cached frames may be stale now.
        this.frameCache.clear();
        this.frameCount = undefined;
        broadcastIpcEvent("session", "storageChanged", null);
      }
      this.storageDataVersion = dataVersion;
    } catch {
      // The storage may be temporarily locked; retry on the next tick.
    } finally {
      this.isPollingStorage = false;
    }
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  // Get the last series.
  private async getSeries() {
    assert(this.storage !== undefined);
    return this.storage.lastSeries();
  }

  /**
   * Get the number of frames.
   */
  public async getFrameCount() {
    const series = await this.getSeries();
    const frameCount = await series.frameCount();
    this.frameCount = frameCount;
    return frameCount;
  }

  /**
   * Get a frame by index. Frames are cached and neighboring frames are
   * prefetched, so scrubbing and playback are usually served from memory.
   */
  public async getFrame(index: number): Promise<Frame> {
    const frame = await this.frameCache.get(index, async () =>
      this.readFrame(index),
    );
    this.prefetchNeighbors(index);
    return frame;
  }

  // Read a frame from the storage in a single native call.
  private async readFrame(index: number): Promise<Frame> {
    const series = await this.getSeries();
    return series.readFrame(index);
  }

  // Opportunistically load the frames around the given one into the cache.
  private prefetchNeighbors(index: number) {
    if (this.frameCount === undefined) return;
    for (const neighbor of [index + 1, index - 1]) {
      if (neighbor < 0 || neighbor >= this.frameCount) continue;
      if (this.frameCache.has(neighbor)) continue;
      this.frameCache
        .get(neighbor, async () => this.readFrame(neighbor))
        .catch(() => {
          // Prefetching is opportunistic; failures resurface on demand.
        });
    }
  }

  /**
   * Export the storage to a directory.
   */
  public async export(dirPath: string) {
    const series = await this.getSeries();
    await series.export(dirPath);
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /**
   * Run the solver.
   */
  public runSolver() {
    this.solver.run(this.workDir);
  }

  /**
   * Stop the solver.
   */
  public stopSolver() {
    this.solver.stop();
  }

  /**
   * Check if the solver is running.
   */
  public isSolverRunning() {
    return this.solver.isRunning();
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

const STORAGE_POLL_INTERVAL_MS = 1000;
const TELEMETRY_INTERVAL_MS = 1000;
const FRAME_CACHE_CAPACITY = 16;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
