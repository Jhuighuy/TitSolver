/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { BrowserWindow, screen } from "electron";
import { beforeEach, describe, expect, it, vi } from "vitest";
import type { ZodType } from "zod";

import type { PersistedState } from "~/main/persisted-state";
import { WindowController, WindowManager } from "~/main/window";

vi.mock("electron");

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// The electron mock exposes an instance registry.
interface MockWindowClass {
  instances: (InstanceType<typeof BrowserWindow> & {
    options: Record<string, unknown>;
  })[];
  reset: () => void;
}

const MockWindow = BrowserWindow as unknown as MockWindowClass;

// A schema-less persisted-state fake backed by a map.
function makePersist(entries: Record<string, unknown> = {}) {
  const data = new Map<string, unknown>(Object.entries(entries));
  return {
    data,
    get: <T>(key: string, _schema: ZodType<T>, fallback?: T) =>
      data.has(key) ? (data.get(key) as T) : fallback,
    set: (key: string, value: unknown) => data.set(key, value),
    withPrefix: () => makePersist(entries),
  } as unknown as PersistedState & { data: Map<string, unknown> };
}

beforeEach(() => {
  vi.clearAllMocks();
  MockWindow.reset();
});

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

describe("WindowController", () => {
  it("opens a window with the persisted geometry", () => {
    const controller = new WindowController(
      "main",
      makePersist({
        "window.x": 100,
        "window.y": 200,
        "window.width": 1600,
        "window.height": 1000,
      }),
    );
    controller.open();

    const window = MockWindow.instances[0];
    expect(window.options).toMatchObject({
      x: 100,
      y: 200,
      width: 1600,
      height: 1000,
    });

    // Opening again reuses the window.
    controller.open();
    expect(MockWindow.instances).toHaveLength(1);
  });

  it("drops an off-screen position and centers instead", () => {
    vi.mocked(screen.getAllDisplays).mockReturnValue([
      { workArea: { x: 0, y: 0, width: 1920, height: 1080 } },
    ] as unknown as Electron.Display[]);

    const controller = new WindowController(
      "main",
      makePersist({ "window.x": 5000, "window.y": 5000 }),
    );
    controller.open();

    const options = MockWindow.instances[0].options;
    expect(options.x).toBeUndefined();
    expect(options.y).toBeUndefined();
  });

  it("persists the geometry on close", () => {
    const persist = makePersist();
    const controller = new WindowController("main", persist);
    controller.open();

    controller.close();
    expect(persist.data.get("window.x")).toBe(10);
    expect(persist.data.get("window.width")).toBe(1280);
    expect(persist.data.get("window.is-maximized")).toBe(false);
  });

  it("applies title and document markers to late-opened windows", () => {
    const controller = new WindowController("main", makePersist());
    controller.setTitle("dam — BlueTit");
    controller.setDocument("/tmp/dam", true);

    controller.open();
    const window = MockWindow.instances[0];
    expect(window.setTitle).toHaveBeenCalledWith("dam — BlueTit");
    expect(window.setRepresentedFilename).toHaveBeenCalledWith("/tmp/dam");
    expect(window.setDocumentEdited).toHaveBeenCalledWith(true);

    // Live updates reach the open window too.
    controller.setDocument(null, false);
    expect(window.setDocumentEdited).toHaveBeenLastCalledWith(false);
  });

  it("ignores page-title updates from the renderer", () => {
    const controller = new WindowController("main", makePersist());
    controller.open();
    const window = MockWindow.instances[0];

    const preventDefault = vi.fn();
    window.emit("page-title-updated", { preventDefault });
    expect(preventDefault).toHaveBeenCalled();
  });
});

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

describe("WindowManager", () => {
  it("opens the main window and finds controllers by window", () => {
    const manager = new WindowManager(makePersist());
    expect(MockWindow.instances).toHaveLength(1);

    const mainWindow = manager.controllers.main.window;
    expect(mainWindow).toBeDefined();
    expect(manager.find(mainWindow as unknown as BrowserWindow)).toBe(
      manager.controllers.main,
    );
  });

  it("refreshes background colors on all open windows", () => {
    const manager = new WindowManager(makePersist());
    manager.updateBackgroundColors();
    expect(MockWindow.instances[0].setBackgroundColor).toHaveBeenCalledTimes(1);
  });
});

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
