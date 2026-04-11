/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { BrowserWindow } from "electron";
import { spawn, type ChildProcessWithoutNullStreams } from "node:child_process";
import { constants as osConstants } from "node:os";
import path from "node:path";

import {
  openStorage as nativeOpenStorage,
  type Storage as NativeStorage,
} from "~/main/bindings";
import { Installation } from "~/main/installation";
import { SESSION_SOLVER_EVENT_CHANNEL } from "~/shared/channels";
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
      this.listener?.({
        kind: "exit",
        code: code ?? 0,
        signal: signal === null ? 0 : osConstants.signals[signal],
      });
    });

    child.on("error", (error) => {
      this.child = undefined;
      this.listener?.({ kind: "stderr", data: String(error) });
      this.listener?.({ kind: "exit", code: 1, signal: 0 });
    });

    this.child = child;
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

  /**
   * Create a session manager.
   */
  public constructor(
    install: Installation,
    private readonly workDir: string,
  ) {
    this.solver = new SolverManager(install);
    this.solver.setEventCallback((event) => {
      for (const window of BrowserWindow.getAllWindows()) {
        window.webContents.send(SESSION_SOLVER_EVENT_CHANNEL, event);
      }
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
  }

  /**
   * Stop the session.
   */
  public stop() {
    this.solver.stop();
    this.storage = undefined;
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  // Get the last series.
  private async getSeries() {
    assert(this.storage !== undefined);
    return await this.storage.lastSeries();
  }

  /**
   * Get the number of frames.
   */
  public async getFrameCount() {
    const series = await this.getSeries();
    return await series.frameCount();
  }

  /**
   * Get a frame by index.
   */
  public async getFrame(index: number): Promise<Frame> {
    const series = await this.getSeries();
    const frame = await series.frame(index);
    const names = await frame.fields();
    return Object.fromEntries(
      await Promise.all(
        names.map(async (name) => {
          const field = await frame.field(name);
          const [type, data] = await Promise.all([field.type(), field.data()]);
          return [name, { type, data }] as const;
        }),
      ),
    );
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
