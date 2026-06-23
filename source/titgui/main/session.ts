/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { spawn, type ChildProcessWithoutNullStreams } from "node:child_process";
import fs from "node:fs";
import { constants as osConstants } from "node:os";
import path from "node:path";

import { BrowserWindow } from "electron";

import {
  openStorage as nativeOpenStorage,
  PropTree as NativePropTree,
  solverSpec as nativeSolverSpec,
  type PropIssue,
  type PropNamespaceTable,
  type PropSpec,
  type PropTreeObject,
  type PropTreeJSON,
  type Storage as NativeStorage,
} from "~/bindings";
import type { Installation } from "~/main/installation";
import { SESSION_SOLVER_EVENT_CHANNEL } from "~/shared/channels";
import type { PropertyDocument } from "~/shared/properties";
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

class ProjectPropertiesManager {
  private readonly spec: PropSpec;
  private tree?: PropTreeObject;
  private issues: PropIssue[] = [];
  private namespaceTable: PropNamespaceTable = {};
  private revision = 0;
  private dirty = false;

  public constructor(private readonly projectPath: string) {
    this.spec = nativeSolverSpec();
  }

  public getDocument(): PropertyDocument {
    if (this.tree === undefined) return this.reload();
    return this.makeDocument();
  }

  public updateTree(tree: PropTreeJSON, revision: number): PropertyDocument {
    this.ensureRevision(revision);
    return this.materialize(NativePropTree.fromJSON(tree), true);
  }

  public save(revision: number): PropertyDocument {
    this.ensureRevision(revision);
    assert(this.tree !== undefined);
    assert(
      this.issues.length === 0,
      "Cannot save project properties with issues.",
    );
    this.tree.saveToFile(this.projectPath);
    this.dirty = false;
    this.revision++;
    return this.makeDocument();
  }

  public reload(): PropertyDocument {
    const tree = fs.existsSync(this.projectPath)
      ? NativePropTree.fromFile(this.projectPath)
      : NativePropTree.fromJSON(null);
    return this.materialize(tree, false);
  }

  private materialize(tree: PropTreeObject, dirty: boolean): PropertyDocument {
    const result = this.spec.materialize(tree);
    this.tree = result.tree;
    this.issues = result.issues;
    this.namespaceTable = result.namespaceTable;
    this.dirty = dirty;
    this.revision++;
    return this.makeDocument();
  }

  private ensureRevision(revision: number) {
    assert(
      revision === this.revision,
      "Project properties were changed by a newer revision.",
    );
  }

  private makeDocument(): PropertyDocument {
    assert(this.tree !== undefined);
    return {
      spec: this.spec.toJSON() as PropertyDocument["spec"],
      tree: this.tree.toJSON() as PropertyDocument["tree"],
      issues: this.issues,
      namespaceTable: this.namespaceTable,
      revision: this.revision,
      dirty: this.dirty,
    };
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
 * A session manager.
 */
export class SessionManager {
  private readonly solver: SolverManager;
  private readonly properties: ProjectPropertiesManager;
  private storage?: NativeStorage;

  /**
   * Create a session manager.
   */
  public constructor(
    install: Installation,
    private readonly workDir: string,
  ) {
    this.solver = new SolverManager(install);
    this.properties = new ProjectPropertiesManager(
      path.join(this.workDir, "project.yaml"),
    );
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
    this.properties.reload();
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
    return this.storage.lastSeries();
  }

  /**
   * Get the number of frames.
   */
  public async getFrameCount() {
    const series = await this.getSeries();
    return series.frameCount();
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

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  public getPropertiesDocument() {
    return this.properties.getDocument();
  }

  public updatePropertiesTree(tree: PropTreeJSON, revision: number) {
    return this.properties.updateTree(tree, revision);
  }

  public saveProperties(revision: number) {
    return this.properties.save(revision);
  }

  public reloadProperties() {
    return this.properties.reload();
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
