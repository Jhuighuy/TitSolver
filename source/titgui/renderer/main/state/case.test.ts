/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { getDefaultStore } from "jotai";
import {
  afterAll,
  beforeAll,
  beforeEach,
  describe,
  expect,
  it,
  vi,
} from "vitest";

import { type FakeIpc, installFakeIpc } from "~/renderer/common/fake-ipc";
import {
  caseDocumentAtom,
  caseSpecAtom,
  caseStateAtom,
  initCaseState,
  recentCasesAtom,
  resetCaseValue,
  saveCase,
  setCaseValue,
} from "~/renderer/main/state/case";
import type { CaseDocument, TreeJson } from "~/shared/case";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

const store = getDefaultStore();

function makeDocument(revision: number, authored: TreeJson): CaseDocument {
  return {
    revision,
    authored,
    materialized: { tree: authored, issues: [], namespaces: {} },
  };
}

// The fake backend: the current document plus a record of submitted edits.
let currentDocument: CaseDocument | null = null;
let updateCalls: { tree: TreeJson; revision: number }[] = [];
let acceptUpdates = true;
let saveCalls = 0;
let fake: FakeIpc;

beforeAll(() => {
  fake = installFakeIpc({
    case: {
      state: () => null,
      recents: () => [{ dir: "/tmp/dam", name: "dam", lastOpenedAt: 1 }],
      getSpec: () => ({ type: "record", fields: [] }),
      document: () => currentDocument,
      updateTree: (_context, tree, revision) => {
        updateCalls.push({ tree, revision });
        return acceptUpdates;
      },
      save: () => {
        saveCalls += 1;
      },
    },
  });
  initCaseState();
});

afterAll(() => {
  fake.uninstall();
});

beforeEach(() => {
  currentDocument = null;
  updateCalls = [];
  acceptUpdates = true;
  saveCalls = 0;
});

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

describe("case state", () => {
  it("pulls the initial spec and recents", async () => {
    await vi.waitFor(() => {
      expect(store.get(caseSpecAtom)).toEqual({ type: "record", fields: [] });
      expect(store.get(recentCasesAtom)).toHaveLength(1);
    });
  });

  it("follows case state events and clears the document on close", async () => {
    store.set(caseDocumentAtom, makeDocument(0, {}));

    fake.emit("case", "caseChanged", {
      dir: "/tmp/dam",
      name: "dam",
      dirty: false,
    });
    await vi.waitFor(() => {
      expect(store.get(caseStateAtom)?.name).toBe("dam");
    });

    fake.emit("case", "caseChanged", null);
    await vi.waitFor(() => {
      expect(store.get(caseStateAtom)).toBeNull();
      expect(store.get(caseDocumentAtom)).toBeNull();
    });
  });

  it("keeps the highest document revision", () => {
    fake.emit("case", "treeChanged", makeDocument(2, { schema: 1 }));
    expect(store.get(caseDocumentAtom)?.revision).toBe(2);

    // A late, older materialization must not win.
    fake.emit("case", "treeChanged", makeDocument(1, { schema: 0 }));
    expect(store.get(caseDocumentAtom)?.revision).toBe(2);

    fake.emit("case", "treeChanged", makeDocument(3, { schema: 1 }));
    expect(store.get(caseDocumentAtom)?.revision).toBe(3);
  });

  it("submits edits against the current revision", async () => {
    fake.emit("case", "treeChanged", makeDocument(4, { schema: 1 }));

    setCaseValue(["fluid", "density"], 900);
    await vi.waitFor(() => {
      expect(updateCalls).toEqual([
        { tree: { schema: 1, fluid: { density: 900 } }, revision: 4 },
      ]);
    });
  });

  it("resets a value by deleting the authored node", async () => {
    fake.emit(
      "case",
      "treeChanged",
      makeDocument(5, { schema: 1, fluid: { density: 900 } }),
    );

    resetCaseValue(["fluid", "density"]);
    await vi.waitFor(() => {
      expect(updateCalls).toEqual([{ tree: { schema: 1 }, revision: 5 }]);
    });
  });

  it("refetches the document after a stale edit", async () => {
    fake.emit("case", "treeChanged", makeDocument(6, { schema: 1 }));

    acceptUpdates = false;
    currentDocument = makeDocument(9, { schema: 1, fresh: true });
    setCaseValue(["schema"], 2);
    await vi.waitFor(() => {
      expect(store.get(caseDocumentAtom)?.revision).toBe(9);
    });
  });

  it("ignores edits when no case is open", () => {
    store.set(caseDocumentAtom, null);
    setCaseValue(["schema"], 2);
    expect(updateCalls).toEqual([]);
  });

  it("saves the case", async () => {
    await saveCase();
    expect(saveCalls).toBe(1);
  });
});

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
