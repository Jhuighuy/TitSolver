/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { EventEmitter } from "node:events";

import {
  app,
  BrowserWindow,
  clipboard,
  Menu,
  net,
  protocol,
  shell,
} from "electron";
import { beforeEach, describe, expect, it, vi } from "vitest";

import { HelpService } from "~/main/help";
import type { Installation } from "~/main/installation";
import { sendIpcEvent } from "~/main/ipc";
import type { WindowController } from "~/main/window";
import type { HelpSession } from "~/shared/help";
import { WEBVIEW_OPEN_IN_TAB_CHANNEL } from "~/shared/webview";

vi.mock("electron");
vi.mock("~/main/ipc", () => ({ sendIpcEvent: vi.fn() }));

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

const install = { manualPath: "/fake/manual" } as Installation;

const HOME = "help://manual/index.html";
const PAGE = "help://manual/guide/setup.html";
const EXTERNAL = "https://example.com/docs";

// A fake window controller with a map-backed persisted state.
function makeController(session?: HelpSession) {
  const persisted = new Map<string, unknown>();
  if (session !== undefined) persisted.set("session", session);
  const window = new BrowserWindow();
  return {
    persist: {
      get: (key: string, _schema: unknown, fallback?: unknown) =>
        persisted.has(key) ? persisted.get(key) : fallback,
      set: (key: string, value: unknown) => persisted.set(key, value),
    },
    persisted,
    window,
    open: vi.fn(),
    close: vi.fn(),
  };
}

function makeService(session?: HelpSession) {
  const controller = makeController(session);
  const service = new HelpService(
    install,
    controller as unknown as WindowController,
  );
  return { service, controller };
}

// A fake guest page (`webview`) web contents, delivered through the app's
// `web-contents-created` hook.
function makeWebviewContents() {
  let windowOpenHandler:
    | ((details: { url: string }) => { action: string })
    | undefined;
  const contents = Object.assign(new EventEmitter(), {
    getType: () => "webview",
    setWindowOpenHandler: (handler: typeof windowOpenHandler) => {
      windowOpenHandler = handler;
    },
    send: vi.fn(),
    loadURL: vi.fn(async () => {}),
    reload: vi.fn(),
    copy: vi.fn(),
    undo: vi.fn(),
    navigationHistory: {
      canGoBack: () => true,
      canGoForward: () => false,
      goBack: vi.fn(),
      goForward: vi.fn(),
    },
  });
  app.emit("web-contents-created", {}, contents);
  return {
    contents,
    openWindow: (url: string) => windowOpenHandler?.({ url }),
  };
}

interface MenuTemplateItem {
  label?: string;
  type?: string;
  enabled?: boolean;
  click?: () => void;
}

function lastMenuTemplate() {
  return vi.mocked(Menu.buildFromTemplate).mock.calls.at(-1)?.[0] as
    | MenuTemplateItem[]
    | undefined;
}

beforeEach(() => {
  vi.clearAllMocks();
  // The electron mock exposes an instance-registry reset.
  (BrowserWindow as unknown as { reset: () => void }).reset();
  // Each service instance registers `web-contents-created`; drop stale
  // listeners left over from earlier tests.
  app.removeAllListeners();
});

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

describe("HelpService session", () => {
  it("starts from the persisted session", () => {
    const { service } = makeService({
      activeTabID: 2,
      tabs: [
        { id: 1, url: HOME },
        { id: 2, url: PAGE },
      ],
    });
    expect(service.getSession().activeTabID).toBe(2);
    expect(service.getSession().tabs).toHaveLength(2);
  });

  it("adds a tab, focuses the window, and broadcasts the change", () => {
    const { service, controller } = makeService();
    service.addTab();

    const session = service.getSession();
    expect(session.tabs).toEqual([{ id: 1, url: HOME }]);
    expect(session.activeTabID).toBe(1);
    expect(controller.open).toHaveBeenCalled();
    expect(controller.persisted.get("session")).toEqual(session);
    expect(sendIpcEvent).toHaveBeenCalledWith(
      controller.window,
      "help",
      "sessionChanged",
      session,
    );
  });

  it("opens external URLs externally instead of tabs", () => {
    const { service } = makeService();
    service.addTab(EXTERNAL);
    expect(shell.openExternal).toHaveBeenCalledWith(EXTERNAL);
    expect(service.getSession().tabs).toHaveLength(0);

    service.addTab(PAGE);
    service.navigateTab(1, EXTERNAL);
    expect(shell.openExternal).toHaveBeenCalledWith(EXTERNAL);
    expect(service.getSession().tabs[0].url).toBe(PAGE);
  });

  it("selects and navigates tabs", () => {
    const { service } = makeService();
    service.addTab();
    service.addTab(PAGE);

    service.selectTab(1);
    expect(service.getSession().activeTabID).toBe(1);

    service.navigateTab(1, PAGE);
    expect(service.getSession().tabs[0].url).toBe(PAGE);
  });

  it("moves the active tab on close and closes the empty window", () => {
    const { service, controller } = makeService();
    service.addTab();
    service.addTab(PAGE);

    service.closeTab(2);
    expect(service.getSession().activeTabID).toBe(1);
    expect(controller.close).not.toHaveBeenCalled();

    service.closeTab(1);
    expect(service.getSession().tabs).toHaveLength(0);
    expect(controller.close).toHaveBeenCalled();
  });
});

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

describe("HelpService webview guests", () => {
  it("denies window opens, forwarding the URL to a tab or the OS", () => {
    makeService();
    const { contents, openWindow } = makeWebviewContents();

    expect(openWindow(PAGE)).toEqual({ action: "deny" });
    expect(contents.send).toHaveBeenCalledWith(
      WEBVIEW_OPEN_IN_TAB_CHANNEL,
      PAGE,
    );

    expect(openWindow(EXTERNAL)).toEqual({ action: "deny" });
    expect(shell.openExternal).toHaveBeenCalledWith(EXTERNAL);
  });

  it("redirects external navigations to the OS browser", () => {
    makeService();
    const { contents } = makeWebviewContents();

    const preventDefault = vi.fn();
    contents.emit("will-navigate", { preventDefault }, EXTERNAL);
    expect(preventDefault).toHaveBeenCalled();
    expect(shell.openExternal).toHaveBeenCalledWith(EXTERNAL);

    // In-manual navigation is left alone.
    contents.emit("will-navigate", { preventDefault: vi.fn() }, PAGE);
    expect(shell.openExternal).toHaveBeenCalledTimes(1);
  });

  it("shows an edit context menu in editable fields", () => {
    makeService();
    const { contents } = makeWebviewContents();

    contents.emit(
      "context-menu",
      {},
      {
        x: 0,
        y: 0,
        linkURL: "",
        isEditable: true,
        editFlags: { canUndo: true, canCopy: false },
      },
    );

    const labels = lastMenuTemplate()?.map((item) => item.label);
    expect(labels).toContain("Undo");
    expect(labels).toContain("Paste");
    lastMenuTemplate()
      ?.find((item) => item.label === "Undo")
      ?.click?.();
    expect(contents.undo).toHaveBeenCalled();
  });

  it("shows a navigation context menu on plain page areas", () => {
    makeService();
    const { contents } = makeWebviewContents();

    contents.emit(
      "context-menu",
      {},
      {
        x: 0,
        y: 0,
        linkURL: "",
        isEditable: false,
        editFlags: { canCopy: true },
      },
    );

    const template = lastMenuTemplate();
    expect(template?.map((item) => item.label)).toEqual(
      expect.arrayContaining(["Back", "Forward", "Reload", "Copy"]),
    );
    expect(template?.find((item) => item.label === "Back")?.enabled).toBe(true);
    expect(template?.find((item) => item.label === "Forward")?.enabled).toBe(
      false,
    );
  });

  it("offers tab opening for manual links and the OS for external ones", () => {
    makeService();
    const { contents } = makeWebviewContents();

    contents.emit(
      "context-menu",
      {},
      {
        x: 0,
        y: 0,
        linkURL: PAGE,
        isEditable: false,
        editFlags: {},
      },
    );
    let template = lastMenuTemplate();
    expect(template?.map((item) => item.label)).toEqual([
      "Open Link",
      "Open Link in New Tab",
    ]);
    template?.find((item) => item.label === "Open Link in New Tab")?.click?.();
    expect(contents.send).toHaveBeenCalledWith(
      WEBVIEW_OPEN_IN_TAB_CHANNEL,
      PAGE,
    );

    contents.emit(
      "context-menu",
      {},
      {
        x: 0,
        y: 0,
        linkURL: EXTERNAL,
        isEditable: false,
        editFlags: {},
      },
    );
    template = lastMenuTemplate();
    expect(template?.map((item) => item.label)).toEqual([
      "Open Link Externally",
      "Copy Link Address",
    ]);
    template?.find((item) => item.label === "Copy Link Address")?.click?.();
    expect(clipboard.writeText).toHaveBeenCalledWith(EXTERNAL);
  });

  it("hardens webview attachment and rejects foreign windows", () => {
    const { controller } = makeService();
    const hostContents = Object.assign(new EventEmitter(), {
      getType: () => "window",
    });
    app.emit("web-contents-created", {}, hostContents);

    // A webview attached to the help window gets locked-down preferences.
    vi.spyOn(BrowserWindow, "fromWebContents").mockReturnValue(
      controller.window,
    );
    const webPreferences: Record<string, unknown> = { preloadURL: "evil" };
    hostContents.emit(
      "will-attach-webview",
      { preventDefault: vi.fn() },
      webPreferences,
    );
    expect(webPreferences.sandbox).toBe(true);
    expect(webPreferences.contextIsolation).toBe(true);
    expect(webPreferences.nodeIntegration).toBe(false);
    expect(webPreferences.preloadURL).toBeUndefined();
    expect(String(webPreferences.preload)).toContain("webview-preload.js");

    // Any other window is refused.
    vi.spyOn(BrowserWindow, "fromWebContents").mockReturnValue(null);
    const preventDefault = vi.fn();
    hostContents.emit("will-attach-webview", { preventDefault }, {});
    expect(preventDefault).toHaveBeenCalled();
  });
});

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

describe("help protocol", () => {
  it("serves manual files and falls back to the 404 page", async () => {
    makeService();
    const handler = vi
      .mocked(protocol.handle)
      .mock.calls.at(-1)?.[1] as (request: { url: string }) => Promise<unknown>;

    await handler({ url: PAGE });
    expect(vi.mocked(net.fetch).mock.calls.at(-1)?.[0]).toContain(
      "/fake/manual/guide/setup.html",
    );

    await handler({ url: "help://elsewhere/etc/passwd" });
    expect(vi.mocked(net.fetch).mock.calls.at(-1)?.[0]).toContain("404.html");
  });
});

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
