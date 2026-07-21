/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import fs from "node:fs";
import path from "node:path";

import { z } from "zod";

import {
  CASE_FILE_NAME,
  type CaseDocument,
  type CaseState,
  type MaterializedCase,
  type RecentCase,
  recentCaseSchema,
  type SpecJson,
  type TreeJson,
} from "~/shared/case";
import { ensure } from "~/shared/utils";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
 * Native case IO, as exposed by the bindings. Injected so the manager can be
 * tested without the native module.
 */
export interface CaseIo {
  caseSpec(): Promise<SpecJson>;
  loadCaseTree(path: string): Promise<TreeJson>;
  saveCaseTree(path: string, tree: TreeJson): Promise<void>;
  materializeCase(tree: TreeJson): Promise<MaterializedCase>;
}

/**
 * Sink for case notifications, wired to IPC broadcasts by the application.
 */
export interface CaseEvents {
  caseChanged(state: CaseState): void;
  treeChanged(document: CaseDocument): void;
}

/**
 * Store for the recents list, satisfied by `PersistedState`.
 */
export interface CaseRecentsStore {
  get<T>(key: string, schema: z.ZodType<T>, fallbackValue: T): T;
  set(key: string, value: unknown): void;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

interface OpenCase {
  dir: string;
  authored: TreeJson;
  revision: number;
  dirty: boolean;
}

/**
 * The open case: its directory, authored tree, and dirty state — plus the
 * recently opened cases list.
 *
 * The authored tree is exactly what the user set (sparse); materialization
 * never mutates it. Saving writes the authored tree only, so defaults are
 * never baked into the case file.
 */
export class CaseManager {
  private current?: OpenCase;
  private spec?: Promise<SpecJson>;

  public constructor(
    private readonly io: CaseIo,
    private readonly persist: CaseRecentsStore,
    private readonly events: CaseEvents,
  ) {}

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /**
   * State of the open case, or `null` when no case is open.
   */
  public state(): CaseState {
    if (this.current === undefined) return null;
    return {
      dir: this.current.dir,
      name: path.basename(this.current.dir),
      dirty: this.current.dirty,
    };
  }

  /**
   * The case specification. The spec is static, so it is fetched once.
   */
  public async getSpec() {
    this.spec ??= this.io.caseSpec();
    return this.spec;
  }

  /**
   * The current case document, or `null` when no case is open.
   */
  public async document(): Promise<CaseDocument | null> {
    if (this.current === undefined) return null;
    return this.makeDocument(this.current);
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /**
   * Create a new case in the given directory and open it.
   */
  public async newCase(dir: string): Promise<CaseState> {
    const casePath = path.join(dir, CASE_FILE_NAME);
    ensure(
      !fs.existsSync(casePath),
      `Directory '${dir}' already contains a case.`,
    );
    fs.mkdirSync(dir, { recursive: true });

    // Author only the schema version; everything else stays at defaults
    // until the user explicitly sets it.
    const { tree } = await this.io.materializeCase(null);
    const schema = z.looseObject({ schema: z.number() }).parse(tree).schema;
    const authored: TreeJson = { schema };
    await this.io.saveCaseTree(casePath, authored);

    return this.finishOpen(dir, authored);
  }

  /**
   * Open the case in the given directory.
   */
  public async openCase(dir: string): Promise<CaseState> {
    const casePath = path.join(dir, CASE_FILE_NAME);
    ensure(
      fs.existsSync(casePath),
      `Directory '${dir}' does not contain a case.`,
    );
    const authored = await this.io.loadCaseTree(casePath);
    return this.finishOpen(dir, authored);
  }

  // Install the opened case, update recents, and notify.
  private async finishOpen(dir: string, authored: TreeJson) {
    this.current = { dir, authored, revision: 0, dirty: false };
    this.touchRecents(dir);

    const state = this.state();
    this.events.caseChanged(state);
    this.events.treeChanged(await this.makeDocument(this.current));
    return state;
  }

  /**
   * Save the authored tree to the case file.
   */
  public async save() {
    const openCase = this.requireOpenCase();
    await this.io.saveCaseTree(
      path.join(openCase.dir, CASE_FILE_NAME),
      openCase.authored,
    );
    openCase.dirty = false;
    this.events.caseChanged(this.state());
  }

  /**
   * Close the case. Unsaved changes are discarded; the caller is responsible
   * for prompting the user beforehand.
   */
  public close() {
    if (this.current === undefined) return;
    this.current = undefined;
    this.events.caseChanged(null);
  }

  /**
   * Replace the authored tree with an edited one. The edit must carry the
   * revision it was based on; a stale revision is rejected (returns `false`)
   * and the caller should retry on top of the latest document.
   */
  public async updateTree(tree: TreeJson, revision: number) {
    const openCase = this.requireOpenCase();
    if (revision !== openCase.revision) return false;

    openCase.authored = tree;
    openCase.revision += 1;
    openCase.dirty = true;

    this.events.caseChanged(this.state());
    this.events.treeChanged(await this.makeDocument(openCase));
    return true;
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /**
   * Recently opened cases, most recent first.
   */
  public recents(): RecentCase[] {
    return this.persist.get(RECENTS_KEY, z.array(recentCaseSchema), []);
  }

  // Move the given directory to the top of the recents list.
  private touchRecents(dir: string) {
    const recents = [
      { dir, name: path.basename(dir), lastOpenedAt: Date.now() },
      ...this.recents().filter((recent) => recent.dir !== dir),
    ].slice(0, MAX_RECENTS);
    this.persist.set(RECENTS_KEY, recents);
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  private requireOpenCase() {
    ensure(this.current !== undefined, "No case is open.");
    return this.current;
  }

  // Materialize the authored tree into a full case document. Note that
  // materialization works on a copy: the authored tree is never mutated, so
  // unknown keys and non-default values survive the round trip to disk.
  private async makeDocument(openCase: OpenCase): Promise<CaseDocument> {
    return {
      revision: openCase.revision,
      authored: openCase.authored,
      materialized: await this.io.materializeCase(openCase.authored),
    };
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

const RECENTS_KEY = "recents";
const MAX_RECENTS = 10;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
