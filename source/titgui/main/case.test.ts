/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import fs from "node:fs";
import os from "node:os";
import path from "node:path";

import { afterEach, beforeEach, describe, expect, it } from "vitest";
import type { z } from "zod";

import { type CaseIo, CaseManager } from "~/main/case";
import type { CaseDocument, CaseState, TreeJson } from "~/shared/case";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Fake native IO: trees are stored as JSON in real files, and the
// materializer fills a single default (`schema: 1`, `value: 0`) and flags
// negative `value`s.
/* oxlint-disable require-await -- some fakes are synchronous. */
const fakeIo: CaseIo = {
  caseSpec: async () => ({ type: "record", fields: [] }),
  loadCaseTree: async (filePath) => {
    const parsed: unknown = JSON.parse(
      await fs.promises.readFile(filePath, "utf8"),
    );
    return parsed as TreeJson;
  },
  saveCaseTree: async (filePath, tree) => {
    await fs.promises.writeFile(filePath, JSON.stringify(tree), "utf8");
  },
  materializeCase: async (tree) => {
    const authored = tree === null ? {} : (tree as Record<string, TreeJson>);
    const value = authored["value"];
    return {
      tree: { schema: 1, value: 0, ...authored },
      issues:
        typeof value === "number" && value < 0
          ? [
              {
                code: "below_minimum" as const,
                path: "/value",
                message: "Value is below minimum 0.",
              },
            ]
          : [],
      namespaces: {},
    };
  },
};
/* oxlint-enable require-await */

function makeFakePersist() {
  const data = new Map<string, unknown>();
  return {
    get<T>(key: string, schema: z.ZodType<T>, fallbackValue: T): T {
      const value = data.get(key);
      if (value === undefined) return fallbackValue;
      return schema.parse(value);
    },
    set(key: string, value: unknown) {
      data.set(key, value);
    },
  };
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

describe("CaseManager", () => {
  let tempDir: string;
  let manager: CaseManager;
  let caseEvents: CaseState[];
  let treeEvents: CaseDocument[];

  beforeEach(() => {
    tempDir = fs.mkdtempSync(path.join(os.tmpdir(), "titgui-case-"));
    caseEvents = [];
    treeEvents = [];
    manager = new CaseManager(fakeIo, makeFakePersist(), {
      caseChanged: (state) => {
        caseEvents.push(state);
      },
      treeChanged: (document) => {
        treeEvents.push(document);
      },
    });
  });

  afterEach(() => {
    fs.rmSync(tempDir, { recursive: true, force: true });
  });

  const caseDir = (name: string) => path.join(tempDir, name);
  const caseFile = (name: string) => path.join(caseDir(name), "case.yaml");
  const readCase = (name: string): unknown =>
    JSON.parse(fs.readFileSync(caseFile(name), "utf8"));

  it("has no open case initially", async () => {
    expect(manager.state()).toBeNull();
    expect(await manager.document()).toBeNull();
    expect(manager.recents()).toEqual([]);
  });

  it("creates a new case authoring only the schema version", async () => {
    const state = await manager.newCase(caseDir("dam"));
    expect(state).toEqual({ dir: caseDir("dam"), name: "dam", dirty: false });
    expect(readCase("dam")).toEqual({ schema: 1 });
    expect(caseEvents).toEqual([state]);
    expect(treeEvents).toHaveLength(1);
    expect(treeEvents[0].revision).toBe(0);
  });

  it("refuses to create a case over an existing one", async () => {
    await manager.newCase(caseDir("dam"));
    await expect(manager.newCase(caseDir("dam"))).rejects.toThrow(
      "already contains a case",
    );
  });

  it("opens a case and pushes the materialized document", async () => {
    fs.mkdirSync(caseDir("dam"), { recursive: true });
    fs.writeFileSync(caseFile("dam"), JSON.stringify({ schema: 1, value: 2 }));

    const state = await manager.openCase(caseDir("dam"));
    expect(state).toEqual({ dir: caseDir("dam"), name: "dam", dirty: false });
    expect(treeEvents).toHaveLength(1);
    expect(treeEvents[0].authored).toEqual({ schema: 1, value: 2 });
    expect(treeEvents[0].materialized.tree).toEqual({ schema: 1, value: 2 });
  });

  it("refuses to open a directory without a case file", async () => {
    await expect(manager.openCase(caseDir("empty"))).rejects.toThrow(
      "does not contain a case",
    );
  });

  it("accepts edits with the current revision and rejects stale ones", async () => {
    await manager.newCase(caseDir("dam"));

    expect(await manager.updateTree({ schema: 1, value: 3 }, 0)).toBe(true);
    expect(manager.state()?.dirty).toBe(true);
    expect(treeEvents).toHaveLength(2);
    expect(treeEvents[1].revision).toBe(1);
    expect(treeEvents[1].materialized.tree).toEqual({ schema: 1, value: 3 });

    // A concurrent edit based on revision 0 is stale now.
    expect(await manager.updateTree({ schema: 1, value: 4 }, 0)).toBe(false);
    expect(treeEvents).toHaveLength(2);
    expect((await manager.document())?.authored).toEqual({
      schema: 1,
      value: 3,
    });
  });

  it("materializes with issues without touching the authored tree", async () => {
    await manager.newCase(caseDir("dam"));
    await manager.updateTree({ schema: 1, value: -1 }, 0);

    const document = await manager.document();
    expect(document?.materialized.issues).toHaveLength(1);
    expect(document?.materialized.issues[0].code).toBe("below_minimum");
    expect(document?.authored).toEqual({ schema: 1, value: -1 });
  });

  it("saves the authored tree only and clears the dirty flag", async () => {
    await manager.newCase(caseDir("dam"));
    await manager.updateTree({ schema: 1, value: 3 }, 0);

    await manager.save();
    expect(manager.state()?.dirty).toBe(false);
    // The materialized default `value: 0` must never reach the file.
    expect(readCase("dam")).toEqual({ schema: 1, value: 3 });
  });

  it("closes the case and notifies", async () => {
    await manager.newCase(caseDir("dam"));
    manager.close();
    expect(manager.state()).toBeNull();
    expect(caseEvents.at(-1)).toBeNull();

    // Closing twice is a no-op.
    manager.close();
    expect(caseEvents.filter((state) => state === null)).toHaveLength(1);
  });

  it("maintains a deduplicated, capped, most-recent-first recents list", async () => {
    await manager.newCase(caseDir("a"));
    await manager.newCase(caseDir("b"));
    await manager.openCase(caseDir("a"));
    await manager.newCase(caseDir("c"));
    expect(manager.recents().map((recent) => recent.name)).toEqual([
      "c",
      "a",
      "b",
    ]);

    for (let index = 0; index < 12; index++) {
      /* oxlint-disable-next-line no-await-in-loop -- opens are sequential. */
      await manager.newCase(caseDir(`case-${index}`));
    }
    expect(manager.recents()).toHaveLength(10);
    expect(manager.recents()[0].name).toBe("case-11");
  });
});

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
