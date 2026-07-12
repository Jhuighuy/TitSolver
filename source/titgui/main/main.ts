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
  type SaveDialogOptions,
} from "electron";
import { z } from "zod";

import {
  caseSpec,
  loadCaseTree,
  materializeCase,
  saveCaseTree,
} from "~/bindings";
import { updateApplicationMenu } from "~/main/app-menu";
import { CaseManager } from "~/main/case";
import { HelpService } from "~/main/help";
import { Installation } from "~/main/installation";
import { broadcastIpcEvent, exposeIpcHandlers } from "~/main/ipc";
import { log } from "~/main/log";
import { PersistedState } from "~/main/persisted-state";
import { SessionManager } from "~/main/session";
import { WindowManager } from "~/main/window";
import type { CaseState } from "~/shared/case";
import { themeSchema } from "~/shared/theme";
import { assert, ensure } from "~/shared/utils";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class Application {
  private install?: Installation;
  private session?: SessionManager;
  private sessionDir?: string;
  private persist?: PersistedState;
  private caseManager?: CaseManager;
  private windowManager?: WindowManager;
  private helpService?: HelpService;

  public run() {
    app.on("ready", () => {
      void this.onReady();
    });
    app.on("window-all-closed", () => {
      this.onWindowAllClosed();
    });
    app.on("activate", () => {
      this.onActivate();
    });
    app.on("will-quit", () => {
      this.onWillQuit();
    });
  }

  private async onReady() {
    try {
      await this.initialize();
    } catch (error) {
      log.error("Failed to start the application:", error);
      dialog.showErrorBox(
        "BlueTit failed to start.",
        error instanceof Error ? error.message : String(error),
      );
      app.exit(1);
    }
  }

  private async initialize() {
    // Find the installation root.
    this.install = Installation.resolve();

    // Test isolation: point all persisted app state at a scratch directory.
    const userDataDir = process.env["TITGUI_USER_DATA"];
    const isolated = userDataDir !== undefined && userDataDir !== "";
    if (isolated) app.setPath("userData", path.resolve(userDataDir));

    // Open persisted state. The repository-root location is legacy; state
    // found there is imported once.
    this.persist = PersistedState.open(
      isolated
        ? undefined
        : path.resolve(this.install.rootPath, "../..", "persisted-state.json"),
    );

    // Setup the case manager. The session is case-scoped: it is created
    // when a case opens and torn down when it closes.
    this.caseManager = new CaseManager(
      { caseSpec, loadCaseTree, saveCaseTree, materializeCase },
      this.persist.withPrefix("case"),
      {
        caseChanged: (state) => {
          this.updateWindowTitle(state);
          this.updateAppMenu(state);
          this.resetSession(state);
          broadcastIpcEvent("case", "caseChanged", state);
        },
        treeChanged: (document) => {
          broadcastIpcEvent("case", "treeChanged", document);
        },
      },
    );

    // Initialize theme.
    nativeTheme.themeSource = this.persist.get("theme", themeSchema, "system");
    nativeTheme.on("updated", () => {
      this.persist?.set("theme", nativeTheme.themeSource);
      this.windowManager?.updateBackgroundColors();
    });

    // Setup windows and the application menu.
    this.windowManager = new WindowManager(this.persist);
    this.updateAppMenu(this.caseManager.state());

    // Setup help.
    this.helpService = new HelpService(
      this.install,
      this.windowManager.controllers.help,
    );

    this.registerIpcHandlers();

    // Development helper: open a case at startup.
    const devCaseDir = process.env["TITGUI_CASE"];
    if (devCaseDir !== undefined && devCaseDir !== "") {
      await this.caseManager.openCase(path.resolve(devCaseDir));
    }
  }

  private onWindowAllClosed() {
    // On macOS the application conventionally stays alive without windows
    // and is reopened from the dock.
    if (process.platform !== "darwin") app.quit();
  }

  private onActivate() {
    this.windowManager?.controllers.main.open();
  }

  private onWillQuit() {
    this.session?.stop();
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  private requireSession() {
    ensure(this.session !== undefined, "No case is open.");
    return this.session;
  }

  // Tear down the session and start a new one for the given case. Called
  // on every case state change; only a directory change causes a reset.
  private resetSession(state: CaseState) {
    if (this.session !== undefined && this.sessionDir === state?.dir) return;
    assert(this.install !== undefined, "Installation is not ready.");

    this.session?.stop();
    this.session = undefined;
    this.sessionDir = undefined;

    if (state !== null) {
      this.session = new SessionManager(this.install, state.dir);
      this.sessionDir = state.dir;
      this.session.start().catch((error: unknown) => {
        log.error("Failed to start the session:", error);
      });
    }

    // The renderer re-reads the frame count and refreshes the viewport.
    broadcastIpcEvent("session", "storageChanged", null);
  }

  private requireCase() {
    assert(this.caseManager !== undefined, "Case manager is not ready.");
    return this.caseManager;
  }

  private updateWindowTitle(state: CaseState) {
    this.windowManager?.controllers.main.setTitle(
      state === null ? "BlueTit" : `${state.name} — BlueTit`,
    );
  }

  // Refresh the application menu (recents, item enablement).
  private updateAppMenu(state: CaseState) {
    updateApplicationMenu(
      {
        newCase: () => {
          runFromMenu(async () => {
            const dir = await this.pickCaseDir(null, true);
            if (dir !== undefined) await this.requireCase().newCase(dir);
          });
        },
        openCase: () => {
          runFromMenu(async () => {
            const dir = await this.pickCaseDir(null, false);
            if (dir !== undefined) await this.requireCase().openCase(dir);
          });
        },
        openRecentCase: (dir) => {
          runFromMenu(async () => this.requireCase().openCase(dir));
        },
        saveCase: () => {
          runFromMenu(async () => this.requireCase().save());
        },
        closeCase: () => {
          this.requireCase().close();
        },
        openHelp: () => {
          this.windowManager?.controllers.help.open();
        },
      },
      { caseState: state, recents: this.caseManager?.recents() ?? [] },
    );
  }

  // Ask the user for a case directory. `create` shows a save-style dialog
  // for a new directory; otherwise an existing directory is picked.
  private async pickCaseDir(window: BrowserWindow | null, create: boolean) {
    if (create) {
      const options = {
        title: "New Case",
        buttonLabel: "Create",
        nameFieldLabel: "Case Name",
        properties: ["createDirectory", "showOverwriteConfirmation"],
      } satisfies SaveDialogOptions;
      const result =
        window === null
          ? await dialog.showSaveDialog(options)
          : await dialog.showSaveDialog(window, options);
      return result.canceled ? undefined : result.filePath;
    }
    const options = {
      title: "Open Case",
      buttonLabel: "Open",
      properties: ["openDirectory"],
    } satisfies OpenDialogOptions;
    const result =
      window === null
        ? await dialog.showOpenDialog(options)
        : await dialog.showOpenDialog(window, options);
    return result.canceled ? undefined : result.filePaths[0];
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

      case: {
        state: () => this.requireCase().state(),
        recents: () => this.requireCase().recents(),
        newCase: async (event) => {
          const dir = await this.pickCaseDir(
            BrowserWindow.fromWebContents(event.sender),
            true,
          );
          if (dir === undefined) return null;
          return this.requireCase().newCase(dir);
        },
        openCase: async (event) => {
          const dir = await this.pickCaseDir(
            BrowserWindow.fromWebContents(event.sender),
            false,
          );
          if (dir === undefined) return null;
          return this.requireCase().openCase(dir);
        },
        openRecent: async (_event, dir) => this.requireCase().openCase(dir),
        save: async () => this.requireCase().save(),
        close: () => {
          this.requireCase().close();
        },
        getSpec: async () => this.requireCase().getSpec(),
        document: async () => this.requireCase().document(),
        updateTree: async (_event, tree, revision) =>
          this.requireCase().updateTree(tree, revision),
      },

      session: {
        // No open case means no session — and simply zero frames.
        frameCount: async () => this.session?.getFrameCount() ?? 0,
        frameTimes: async () => this.session?.getFrameTimes() ?? [],
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
        runSolver: async () => {
          // Running requires a clean materialization; the solver would
          // reject an invalid case file anyway, but failing here is clearer.
          const document = await this.requireCase().document();
          ensure(document !== null, "No case is open.");
          ensure(
            document.materialized.issues.length === 0,
            "The case has validation issues; fix them before running.",
          );
          this.requireSession().runSolver();
        },
        stopSolver: () => {
          this.requireSession().stopSolver();
        },
        isSolverRunning: () => this.session?.isSolverRunning() ?? false,
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

// Run a menu-triggered action: unlike IPC calls, there is no renderer
// promise to reject into, so failures surface as a dialog.
function runFromMenu(action: () => Promise<unknown>) {
  action().catch((error: unknown) => {
    log.error("Menu action failed:", error);
    dialog.showErrorBox(
      "Action failed.",
      error instanceof Error ? error.message : String(error),
    );
  });
}

new Application().run();

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
