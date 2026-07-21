/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// Manual mock of the `electron` module for main-process unit tests: enough
// surface for dialogs, windows, menus, and app lifecycle. Activated with
// `vi.mock("electron")` in a test file.

import { EventEmitter } from "node:events";

import { vi } from "vitest";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export const app = Object.assign(new EventEmitter(), {
  getPath: vi.fn(() => "/tmp/titgui-electron-mock"),
  setPath: vi.fn(),
  getName: vi.fn(() => "BlueTit"),
  quit: vi.fn(),
  exit: vi.fn(),
});

export const dialog = {
  showMessageBox: vi.fn(),
  showMessageBoxSync: vi.fn(),
  showOpenDialog: vi.fn(),
  showOpenDialogSync: vi.fn(),
  showSaveDialog: vi.fn(),
  showErrorBox: vi.fn(),
};

export const nativeTheme = Object.assign(new EventEmitter(), {
  themeSource: "system",
  shouldUseDarkColors: false,
});

export const screen = {
  getAllDisplays: vi.fn(() => [
    { workArea: { x: 0, y: 0, width: 1920, height: 1080 } },
  ]),
};

export const Menu = {
  buildFromTemplate: vi.fn((template: unknown[]) => ({
    items: template,
    popup: vi.fn(),
  })),
  setApplicationMenu: vi.fn(),
  getApplicationMenu: vi.fn(() => null),
};

export const ipcMain = {
  handle: vi.fn(),
  removeHandler: vi.fn(),
};

export const protocol = {
  handle: vi.fn(),
  registerSchemesAsPrivileged: vi.fn(),
};

export const net = {
  fetch: vi.fn(async () => new Response("")),
};

export const shell = {
  openExternal: vi.fn(async () => {}),
};

export const clipboard = {
  clear: vi.fn(),
  writeText: vi.fn(),
  readText: vi.fn(() => ""),
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
 * A stand-in browser window: an event emitter with spied methods and just
 * enough state (bounds, full-screen) for the window-controller logic.
 */
export class BrowserWindow extends EventEmitter {
  public static instances: BrowserWindow[] = [];

  public readonly options: unknown;
  public webContents = { send: vi.fn() };

  public setTitle = vi.fn();
  public setRepresentedFilename = vi.fn();
  public setDocumentEdited = vi.fn();
  public setBackgroundColor = vi.fn();
  public loadFile = vi.fn(async () => {});
  public loadURL = vi.fn(async () => {});
  public show = vi.fn();
  public focus = vi.fn();
  public close = vi.fn(() => {
    this.emit("close");
    this.emit("closed");
  });
  public maximize = vi.fn();
  public setFullScreen = vi.fn();
  public isMaximized = vi.fn(() => false);
  public isFullScreen = vi.fn(() => false);
  public getNormalBounds = vi.fn(() => ({
    x: 10,
    y: 20,
    width: 1280,
    height: 800,
  }));

  public constructor(options?: unknown) {
    super();
    this.options = options;
    BrowserWindow.instances.push(this);
  }

  public static getAllWindows() {
    return BrowserWindow.instances;
  }

  public static fromWebContents() {
    return null;
  }

  /** Reset the instance registry between tests. */
  public static reset() {
    BrowserWindow.instances = [];
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
