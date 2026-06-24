/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import path from "node:path";

import {
  app,
  BrowserWindow,
  dialog,
  ipcMain,
  nativeTheme,
  type OpenDialogOptions,
} from "electron";
import { z } from "zod";

import { makeHelpService } from "~/main/help";
import { Installation } from "~/main/installation";
import { PersistedState } from "~/main/persisted-state";
import { SessionManager } from "~/main/session";
import { WindowManager } from "~/main/window";
import {
  HELP_ADD_TAB_CHANNEL,
  HELP_CLOSE_TAB_CHANNEL,
  HELP_GET_SESSION_CHANNEL,
  HELP_NAVIGATE_TAB_CHANNEL,
  HELP_SELECT_TAB_CHANNEL,
  HELP_SESSION_CHANGED_CHANNEL,
  SESSION_EXPORT_RUN_CHANNEL,
  SESSION_FRAME_COUNT_CHANNEL,
  SESSION_FRAME_GET_CHANNEL,
  SESSION_SOLVER_IS_RUNNING_CHANNEL,
  SESSION_SOLVER_RUN_CHANNEL,
  SESSION_SOLVER_STOP_CHANNEL,
  THEME_GET_CHANNEL,
  THEME_SET_CHANNEL,
  WINDOW_IS_FULL_SCREEN_CHANNEL,
  WINDOW_PERSIST_GET_CHANNEL,
  WINDOW_PERSIST_SET_CHANNEL,
} from "~/shared/channels";
import type { HelpService } from "~/shared/help";
import { type Theme, themeSchema } from "~/shared/theme";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class Application {
  private install?: Installation;
  private session?: SessionManager;
  private persist?: PersistedState;
  private windowManager?: WindowManager;
  private helpManager?: HelpService;

  public run() {
    app.on("ready", () => {
      void this.onReady();
    });
    app.on("window-all-closed", () => {
      this.onWindowAllClosed();
    });
    app.on("will-quit", () => {
      this.onWillQuit();
    });
  }

  private async onReady() {
    // Find the installation root.
    this.install = Installation.resolve();

    // Work directory is hardcoded at the moment.
    const workDir = path.resolve(this.install.rootPath, "../..");

    // Start the session.
    this.session = new SessionManager(this.install, workDir);
    await this.session.start();

    // Load persisted state.
    this.persist = PersistedState.load(
      path.join(workDir, "persisted-state.json"),
    );

    // Initialize theme.
    nativeTheme.themeSource = this.persist.get("theme", themeSchema, "system");
    nativeTheme.on("updated", () => {
      this.persist?.set("theme", nativeTheme.themeSource);
      this.windowManager?.updateBackgroundColors();
    });

    // Setup windows.
    this.windowManager = new WindowManager(this.persist);

    // Setup help.
    this.helpManager = makeHelpService(
      this.install,
      this.windowManager.controllers.help,
    );

    this.registerIpcHandlers();
  }

  private onWindowAllClosed() {
    app.quit();
  }

  private onWillQuit() {
    this.session?.stop();
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

    ipcMain.removeHandler(SESSION_FRAME_COUNT_CHANNEL);
    ipcMain.handle(SESSION_FRAME_COUNT_CHANNEL, async () => {
      return this.session?.getFrameCount();
    });

    ipcMain.removeHandler(SESSION_FRAME_GET_CHANNEL);
    ipcMain.handle(SESSION_FRAME_GET_CHANNEL, async (_event, index: number) => {
      return this.session?.getFrame(index);
    });

    ipcMain.removeHandler(SESSION_EXPORT_RUN_CHANNEL);
    ipcMain.handle(SESSION_EXPORT_RUN_CHANNEL, async (event) => {
      const options: OpenDialogOptions = {
        title: "Export Data",
        buttonLabel: "Export",
        properties: ["openDirectory", "createDirectory"],
      };
      const window = BrowserWindow.fromWebContents(event.sender);
      const result =
        window === null
          ? await dialog.showOpenDialog(options)
          : await dialog.showOpenDialog(window, options);
      if (result.canceled) return;
      await this.session?.export(result.filePaths[0]);
    });

    ipcMain.removeHandler(SESSION_SOLVER_RUN_CHANNEL);
    ipcMain.handle(SESSION_SOLVER_RUN_CHANNEL, () => {
      this.session?.runSolver();
    });

    ipcMain.removeHandler(SESSION_SOLVER_STOP_CHANNEL);
    ipcMain.handle(SESSION_SOLVER_STOP_CHANNEL, () => {
      this.session?.stopSolver();
    });

    ipcMain.removeHandler(SESSION_SOLVER_IS_RUNNING_CHANNEL);
    ipcMain.handle(SESSION_SOLVER_IS_RUNNING_CHANNEL, () => {
      return this.session?.isSolverRunning();
    });

    ipcMain.removeHandler(HELP_GET_SESSION_CHANNEL);
    ipcMain.handle(HELP_GET_SESSION_CHANNEL, async () => {
      return this.helpManager?.getSession();
    });

    this.helpManager?.onSessionChanged((session) => {
      this.windowManager?.controllers.help?.window?.webContents.send(
        HELP_SESSION_CHANGED_CHANNEL,
        session,
      );
    });

    ipcMain.removeHandler(HELP_ADD_TAB_CHANNEL);
    ipcMain.handle(HELP_ADD_TAB_CHANNEL, async (_event, url?: string) => {
      return this.helpManager?.addTab(url);
    });

    ipcMain.removeHandler(HELP_CLOSE_TAB_CHANNEL);
    ipcMain.handle(HELP_CLOSE_TAB_CHANNEL, async (_event, id: number) => {
      return this.helpManager?.closeTab(id);
    });

    ipcMain.removeHandler(HELP_SELECT_TAB_CHANNEL);
    ipcMain.handle(HELP_SELECT_TAB_CHANNEL, async (_event, id: number) => {
      return this.helpManager?.selectTab(id);
    });

    ipcMain.removeHandler(HELP_NAVIGATE_TAB_CHANNEL);
    ipcMain.handle(
      HELP_NAVIGATE_TAB_CHANNEL,
      async (_event, id: number, url?: string) => {
        return this.helpManager?.navigateTab(id, url);
      },
    );
  }
}

new Application().run();

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
