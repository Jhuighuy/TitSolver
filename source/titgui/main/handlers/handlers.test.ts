/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { dialog, nativeTheme, type IpcMainInvokeEvent } from "electron";
import { beforeEach, describe, expect, it, vi } from "vitest";

import type { CaseManager } from "~/main/case";
import type { CaseFlows } from "~/main/case-flows";
import { createCaseHandlers } from "~/main/handlers/case";
import { createHelpHandlers } from "~/main/handlers/help";
import { createSessionHandlers } from "~/main/handlers/session";
import { createThemeHandlers } from "~/main/handlers/theme";
import { createWindowHandlers } from "~/main/handlers/window";
import type { HelpService } from "~/main/help";
import type { SessionManager } from "~/main/session";
import type { WindowController } from "~/main/window";
import type { CaseDocument } from "~/shared/case";

vi.mock("electron");

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Handlers receive a real invoke event in production; the fakes only need
// a sender reference.
const event = { sender: {} } as IpcMainInvokeEvent;

function makeDocument(issueCount: number): CaseDocument {
  return {
    revision: 0,
    authored: {},
    materialized: {
      tree: {},
      issues: Array.from({ length: issueCount }, (_item, index) => ({
        code: "invalid_value" as const,
        path: `/bad-${index.toString()}`,
        message: "Bad.",
      })),
      namespaces: {},
    },
  };
}

beforeEach(() => {
  vi.clearAllMocks();
});

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

describe("theme handlers", () => {
  it("read and write the native theme source", async () => {
    const handlers = createThemeHandlers();
    expect(handlers.get(event)).toBe("system");
    await handlers.set(event, "dark");
    expect(nativeTheme.themeSource).toBe("dark");
    nativeTheme.themeSource = "system";
  });
});

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

describe("window handlers", () => {
  it("delegate persisted state to the window controller", async () => {
    const persist = { get: vi.fn(() => 42), set: vi.fn() };
    const handlers = createWindowHandlers(
      () => ({ persist }) as unknown as WindowController,
    );

    expect(handlers.persistGet(event, "key")).toBe(42);
    await handlers.persistSet(event, "key", 7);
    expect(persist.set).toHaveBeenCalledWith("key", 7);
  });

  it("tolerate a missing window controller", () => {
    const handlers = createWindowHandlers(() => undefined);
    expect(handlers.persistGet(event, "key")).toBeUndefined();
    expect(() => {
      void handlers.persistSet(event, "key", 7);
    }).not.toThrow();
    expect(handlers.isFullScreen(event)).toBe(false);
  });
});

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

describe("case handlers", () => {
  it("route reads to the manager and dialog flows to the case flows", async () => {
    const caseManager = {
      state: vi.fn(() => null),
      recents: vi.fn(() => []),
      save: vi.fn(async () => {}),
      getSpec: vi.fn(async () => ({ type: "record" as const, fields: [] })),
      document: vi.fn(async () => null),
      updateTree: vi.fn(async () => true),
    };
    const flows = {
      newCase: vi.fn(async () => null),
      openCase: vi.fn(async () => null),
      openRecent: vi.fn(async () => null),
      close: vi.fn(async () => {}),
    };
    const handlers = createCaseHandlers(
      () => caseManager as unknown as CaseManager,
      () => flows as unknown as CaseFlows,
    );

    expect(handlers.state(event)).toBeNull();
    expect(handlers.recents(event)).toEqual([]);
    await handlers.newCase(event);
    expect(flows.newCase).toHaveBeenCalled();
    await handlers.openRecent(event, "/tmp/dam");
    expect(flows.openRecent).toHaveBeenCalledWith(null, "/tmp/dam");
    await handlers.close(event);
    expect(flows.close).toHaveBeenCalled();
    expect(await handlers.updateTree(event, {}, 0)).toBe(true);
  });
});

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

describe("session handlers", () => {
  function makeSession() {
    return {
      getFrameCount: vi.fn(async () => 3),
      getFrameTimes: vi.fn(async () => [0, 1, 2]),
      getFrame: vi.fn(async () => ({})),
      export: vi.fn(async () => {}),
      runSolver: vi.fn(),
      stopSolver: vi.fn(),
      isSolverRunning: vi.fn(() => true),
    };
  }

  function makeCase(issueCount: number) {
    return { document: vi.fn(async () => makeDocument(issueCount)) };
  }

  it("report the empty state without a session", async () => {
    const handlers = createSessionHandlers(
      () => undefined,
      () => makeCase(0) as unknown as CaseManager,
    );
    expect(await handlers.frameCount(event)).toBe(0);
    expect(await handlers.frameTimes(event)).toEqual([]);
    expect(handlers.isSolverRunning(event)).toBe(false);
    await expect(handlers.frame(event, 0)).rejects.toThrow("No case is open.");
  });

  it("delegate to a live session", async () => {
    const session = makeSession();
    const handlers = createSessionHandlers(
      () => session as unknown as SessionManager,
      () => makeCase(0) as unknown as CaseManager,
    );
    expect(await handlers.frameCount(event)).toBe(3);
    expect(await handlers.frameTimes(event)).toEqual([0, 1, 2]);
    expect(handlers.isSolverRunning(event)).toBe(true);
    await handlers.stopSolver(event);
    expect(session.stopSolver).toHaveBeenCalled();
  });

  it("gate solver runs on a clean materialization", async () => {
    const session = makeSession();
    const dirtyHandlers = createSessionHandlers(
      () => session as unknown as SessionManager,
      () => makeCase(2) as unknown as CaseManager,
    );
    await expect(dirtyHandlers.runSolver(event)).rejects.toThrow(
      "validation issues",
    );
    expect(session.runSolver).not.toHaveBeenCalled();

    const cleanHandlers = createSessionHandlers(
      () => session as unknown as SessionManager,
      () => makeCase(0) as unknown as CaseManager,
    );
    await cleanHandlers.runSolver(event);
    expect(session.runSolver).toHaveBeenCalledTimes(1);
  });

  it("export via the directory dialog", async () => {
    const session = makeSession();
    const handlers = createSessionHandlers(
      () => session as unknown as SessionManager,
      () => makeCase(0) as unknown as CaseManager,
    );

    vi.mocked(dialog.showOpenDialog).mockResolvedValue({
      canceled: true,
      filePaths: [],
      bookmarks: [],
    });
    await handlers.export(event);
    expect(session.export).not.toHaveBeenCalled();

    vi.mocked(dialog.showOpenDialog).mockResolvedValue({
      canceled: false,
      filePaths: ["/tmp/out"],
      bookmarks: [],
    });
    await handlers.export(event);
    expect(session.export).toHaveBeenCalledWith("/tmp/out");
  });
});

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

describe("help handlers", () => {
  it("delegate to the help service", async () => {
    const help = {
      getSession: vi.fn(() => ({ tabs: [] })),
      addTab: vi.fn(),
      closeTab: vi.fn(),
      selectTab: vi.fn(),
      navigateTab: vi.fn(),
    };
    const handlers = createHelpHandlers(() => help as unknown as HelpService);

    expect(handlers.getSession(event)).toEqual({ tabs: [] });
    await handlers.addTab(event, "help://manual/");
    expect(help.addTab).toHaveBeenCalledWith("help://manual/");
    await handlers.closeTab(event, 1);
    await handlers.selectTab(event, 2);
    await handlers.navigateTab(event, 3, "help://manual/x");
    expect(help.navigateTab).toHaveBeenCalledWith(3, "help://manual/x");
  });
});

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
