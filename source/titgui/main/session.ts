/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { spawn, type ChildProcessWithoutNullStreams } from "node:child_process";
import { constants as osConstants } from "node:os";
import path from "node:path";

import { BrowserWindow } from "electron";

import { openRun as nativeOpenRun, type Run as NativeRun } from "~/bindings";
import type { Installation } from "~/main/installation";
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
  private run?: NativeRun;
  private readonly runPath: string;

  /**
   * Create a session manager.
   */
  public constructor(
    install: Installation,
    private readonly workDir: string,
  ) {
    this.runPath = path.join(workDir, "particles.tit-run");
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
    // Opening a path-backed handle is valid before the solver publishes the
    // first frame; frameCount() returns zero until the run index is available.
    this.run = await nativeOpenRun(this.runPath);
  }

  /**
   * Stop the session.
   */
  public stop() {
    this.solver.stop();
    this.run = undefined;
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /** Get the number of committed frames. */
  public async getFrameCount() {
    assert(this.run !== undefined);
    return this.run.frameCount();
  }

  /**
   * Get a frame by index.
   */
  public async getFrame(index: number): Promise<Frame> {
    assert(this.run !== undefined);
    const frame = await this.run.frame(index);
    const names = await frame.fields();
    const result: Frame = {};
    await Promise.all(
      names.map(async (name) => {
        const field = await frame.field(name);
        const [type, data] = await Promise.all([field.type(), field.data()]);

        // Stable IDs remain lossless BigUint64Array values in the native API.
        // The current WebGL field map is numeric-only, so identity fields are
        // not forwarded into its scalar/vector visualization path.
        const kind = type.kind;
        if (kind === "int64_t" || kind === "uint64_t") return;
        if (data instanceof BigInt64Array || data instanceof BigUint64Array) {
          return;
        }
        result[name] = { type: { ...type, kind }, data };
      }),
    );
    return result;
  }

  /** Export a consistent snapshot of the committed run. */
  public async export(dirPath: string) {
    assert(this.run !== undefined);
    const destination = path.join(dirPath, path.basename(this.runPath));
    await this.run.export(destination);
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
