/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { app, BrowserWindow, ipcMain, nativeTheme } from "electron";
import path from "node:path";
import { z } from "zod";

import { Installation } from "~/main/installation";
import { PersistedState } from "~/main/persisted-state";
import { ServerManager } from "~/main/server";
import { WindowManager } from "~/main/window";
import {
  HELP_OPEN_CHANNEL,
  THEME_GET_CHANNEL,
  THEME_SET_CHANNEL,
  WINDOW_IS_FULL_SCREEN_CHANNEL,
  WINDOW_PERSIST_GET_CHANNEL,
  WINDOW_PERSIST_SET_CHANNEL,
} from "~/shared/channels";
import { Theme, themeSchema } from "~/shared/theme";
import { assert } from "~/shared/utils";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class Application {
  private install?: Installation;
  private server?: ServerManager;
  private persist?: PersistedState;
  private windowManager?: WindowManager;

  public run() {
    app.on("ready", () => {
      this.onReady();
    });
    app.on("window-all-closed", () => {
      this.onWindowAllClosed();
    });
    app.on("will-quit", () => {
      this.onWillQuit();
    });
  }

  private onReady() {
    // Find the installation root.
    this.install = Installation.resolve();
    const workDir = path.resolve(this.install.rootPath, "../..");

    // Start the backend.
    this.server = new ServerManager(this.install);
    this.server.start(workDir);

    // Load persisted state.
    this.persist = PersistedState.load(
      path.join(workDir, "persisted-state.json"),
    );

    // Initialize theme.
    nativeTheme.themeSource = this.persist.get("theme", themeSchema, "system");
    nativeTheme.on("updated", () => {
      this.persist?.set("theme", nativeTheme.themeSource);
    });

    // Setup windows.
    this.windowManager = new WindowManager(this.persist);

    this.registerIpcHandlers();
  }

  private onWindowAllClosed() {
    app.quit();
  }

  private onWillQuit() {
    this.server?.stop();
    this.persist?.save();
  }

  private registerIpcHandlers() {
    ipcMain.removeHandler(THEME_GET_CHANNEL);
    ipcMain.handle(THEME_GET_CHANNEL, () => {
      return nativeTheme.themeSource;
    });

    ipcMain.removeHandler(THEME_SET_CHANNEL);
    ipcMain.handle(THEME_SET_CHANNEL, (_event, theme: Theme) => {
      nativeTheme.themeSource = theme;
    });

    ipcMain.removeHandler(WINDOW_PERSIST_GET_CHANNEL);
    ipcMain.handle(WINDOW_PERSIST_GET_CHANNEL, (event, key: string) => {
      const window = BrowserWindow.fromWebContents(event.sender);
      if (window === null) return;
      return this.windowManager?.find(window)?.persist.get(key, z.unknown());
    });

    ipcMain.removeHandler(WINDOW_PERSIST_SET_CHANNEL);
    ipcMain.handle(
      WINDOW_PERSIST_SET_CHANNEL,
      (event, key: string, value: unknown) => {
        const window = BrowserWindow.fromWebContents(event.sender);
        if (window === null) return;
        this.windowManager?.find(window)?.persist.set(key, value);
      },
    );

    ipcMain.removeHandler(WINDOW_IS_FULL_SCREEN_CHANNEL);
    ipcMain.handle(WINDOW_IS_FULL_SCREEN_CHANNEL, (event) => {
      const window = BrowserWindow.fromWebContents(event.sender);
      return window?.isFullScreen() ?? false;
    });

    ipcMain.removeHandler(HELP_OPEN_CHANNEL);
    ipcMain.handle(HELP_OPEN_CHANNEL, (_event, path?: string) => {
      assert(path === undefined);
      this.windowManager?.controllers.help.open();
    });
  }
}

new Application().run();

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
