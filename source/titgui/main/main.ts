/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import path from "node:path";

import {
  app,
  BrowserWindow,
  dialog,
  type IpcMainInvokeEvent,
  nativeTheme,
  type OpenDialogOptions,
} from "electron";
import { z } from "zod";

import { HelpService } from "~/main/help";
import { Installation } from "~/main/installation";
import { exposeIpcHandlers } from "~/main/ipc";
import { PersistedState } from "~/main/persisted-state";
import { SessionManager } from "~/main/session";
import { WindowManager } from "~/main/window";
import { themeSchema } from "~/shared/theme";
import { assert } from "~/shared/utils";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class Application {
  private install?: Installation;
  private session?: SessionManager;
  private persist?: PersistedState;
  private windowManager?: WindowManager;
  private helpService?: HelpService;

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
    this.helpService = new HelpService(
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

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  private requireSession() {
    assert(this.session !== undefined, "Session is not ready.");
    return this.session;
  }

  private requireHelp() {
    assert(this.helpService !== undefined, "Help service is not ready.");
    return this.helpService;
  }

  private findWindowController(event: IpcMainInvokeEvent) {
    const window = BrowserWindow.fromWebContents(event.sender);
    if (window === null) return;
    return this.windowManager?.find(window);
  }

  private registerIpcHandlers() {
    exposeIpcHandlers({
      theme: {
        get: () => nativeTheme.themeSource,
        set: (_event, theme) => {
          nativeTheme.themeSource = theme;
        },
      },

      window: {
        persistGet: (event, key) => {
          return this.findWindowController(event)?.persist.get(
            key,
            z.unknown(),
          );
        },
        persistSet: (event, key, value) => {
          this.findWindowController(event)?.persist.set(key, value);
        },
        isFullScreen: (event) => {
          const window = BrowserWindow.fromWebContents(event.sender);
          return window?.isFullScreen() ?? false;
        },
      },

      session: {
        frameCount: async () => this.requireSession().getFrameCount(),
        frame: async (_event, index) => this.requireSession().getFrame(index),
        export: async (event) => {
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
          await this.requireSession().export(result.filePaths[0]);
        },
        runSolver: () => {
          this.requireSession().runSolver();
        },
        stopSolver: () => {
          this.requireSession().stopSolver();
        },
        isSolverRunning: () => this.requireSession().isSolverRunning(),
      },

      help: {
        getSession: () => this.requireHelp().getSession(),
        addTab: (_event, url) => {
          this.requireHelp().addTab(url);
        },
        closeTab: (_event, id) => {
          this.requireHelp().closeTab(id);
        },
        selectTab: (_event, id) => {
          this.requireHelp().selectTab(id);
        },
        navigateTab: (_event, id, url) => {
          this.requireHelp().navigateTab(id, url);
        },
      },
    });
  }
}

new Application().run();

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
