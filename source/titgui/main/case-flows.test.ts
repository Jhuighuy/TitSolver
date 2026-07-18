/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { dialog } from "electron";
import { beforeEach, describe, expect, it, vi } from "vitest";

import type { CaseManager } from "~/main/case";
import { CaseFlows } from "~/main/case-flows";
import type { CaseState } from "~/shared/case";

vi.mock("electron");

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// A fake case manager tracking the operations the flows delegate to.
function makeCaseManager(state: CaseState) {
  return {
    state: vi.fn(() => state),
    save: vi.fn(async () => {}),
    close: vi.fn(),
    newCase: vi.fn(async (dir: string) => ({ dir, name: "new", dirty: false })),
    openCase: vi.fn(async (dir: string) => ({
      dir,
      name: "opened",
      dirty: false,
    })),
  };
}

function makeFlows(state: CaseState) {
  const caseManager = makeCaseManager(state);
  return {
    caseManager,
    flows: new CaseFlows(caseManager as unknown as CaseManager),
  };
}

const CLEAN: CaseState = { dir: "/tmp/dam", name: "dam", dirty: false };
const DIRTY: CaseState = { dir: "/tmp/dam", name: "dam", dirty: true };

const messageBox = vi.mocked(dialog.showMessageBox);
const openDialog = vi.mocked(dialog.showOpenDialog);
const saveDialog = vi.mocked(dialog.showSaveDialog);

function answerPrompt(response: number) {
  messageBox.mockResolvedValue({ response, checkboxChecked: false });
}

beforeEach(() => {
  vi.clearAllMocks();
});

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

describe("CaseFlows.confirmDiscard", () => {
  it("passes through for a clean case without prompting", async () => {
    const { flows } = makeFlows(CLEAN);
    expect(await flows.confirmDiscard(null)).toBe(true);
    expect(messageBox).not.toHaveBeenCalled();
  });

  it("saves and proceeds on Save", async () => {
    const { flows, caseManager } = makeFlows(DIRTY);
    answerPrompt(0);
    expect(await flows.confirmDiscard(null)).toBe(true);
    expect(caseManager.save).toHaveBeenCalledTimes(1);
  });

  it("proceeds without saving on Don't Save", async () => {
    const { flows, caseManager } = makeFlows(DIRTY);
    answerPrompt(1);
    expect(await flows.confirmDiscard(null)).toBe(true);
    expect(caseManager.save).not.toHaveBeenCalled();
  });

  it("halts on Cancel", async () => {
    const { flows, caseManager } = makeFlows(DIRTY);
    answerPrompt(2);
    expect(await flows.confirmDiscard(null)).toBe(false);
    expect(caseManager.save).not.toHaveBeenCalled();
  });
});

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

describe("CaseFlows", () => {
  it("creates a case in the picked directory", async () => {
    const { flows, caseManager } = makeFlows(CLEAN);
    saveDialog.mockResolvedValue({ canceled: false, filePath: "/tmp/new" });

    const state = await flows.newCase(null);
    expect(caseManager.newCase).toHaveBeenCalledWith("/tmp/new");
    expect(state?.dir).toBe("/tmp/new");
  });

  it("returns null when the directory pick is cancelled", async () => {
    const { flows, caseManager } = makeFlows(CLEAN);
    saveDialog.mockResolvedValue({ canceled: true, filePath: "" });

    expect(await flows.newCase(null)).toBeNull();
    expect(caseManager.newCase).not.toHaveBeenCalled();
  });

  it("guards a dirty case before opening another one", async () => {
    const { flows, caseManager } = makeFlows(DIRTY);
    answerPrompt(2);

    expect(await flows.openCase(null)).toBeNull();
    expect(openDialog).not.toHaveBeenCalled();
    expect(caseManager.openCase).not.toHaveBeenCalled();
  });

  it("opens the picked directory", async () => {
    const { flows, caseManager } = makeFlows(CLEAN);
    openDialog.mockResolvedValue({
      canceled: false,
      filePaths: ["/tmp/other"],
      bookmarks: [],
    });

    const state = await flows.openCase(null);
    expect(caseManager.openCase).toHaveBeenCalledWith("/tmp/other");
    expect(state?.dir).toBe("/tmp/other");
  });

  it("opens a recent directory, guarded", async () => {
    const dirty = makeFlows(DIRTY);
    answerPrompt(2);
    expect(await dirty.flows.openRecent(null, "/tmp/recent")).toBeNull();

    const clean = makeFlows(CLEAN);
    const state = await clean.flows.openRecent(null, "/tmp/recent");
    expect(clean.caseManager.openCase).toHaveBeenCalledWith("/tmp/recent");
    expect(state?.dir).toBe("/tmp/recent");
  });

  it("closes only when the guard allows it", async () => {
    const halted = makeFlows(DIRTY);
    answerPrompt(2);
    await halted.flows.close(null);
    expect(halted.caseManager.close).not.toHaveBeenCalled();

    const discarded = makeFlows(DIRTY);
    answerPrompt(1);
    await discarded.flows.close(null);
    expect(discarded.caseManager.close).toHaveBeenCalledTimes(1);
  });
});

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
