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
} from "electron";

import {
  caseSpec,
  loadCaseTree,
  materializeCase,
  saveCaseTree,
} from "~/bindings";
import { updateApplicationMenu } from "~/main/app-menu";
import { CaseManager } from "~/main/case";
import { CaseFlows } from "~/main/case-flows";
import { createCaseHandlers } from "~/main/handlers/case";
import { createHelpHandlers } from "~/main/handlers/help";
import { createSessionHandlers } from "~/main/handlers/session";
import { createThemeHandlers } from "~/main/handlers/theme";
import { createWindowHandlers } from "~/main/handlers/window";
import { HelpService } from "~/main/help";
import { Installation } from "~/main/installation";
import { broadcastIpcEvent, exposeIpcHandlers } from "~/main/ipc";
import { log } from "~/main/log";
import { PersistedState } from "~/main/persisted-state";
import { SessionManager } from "~/main/session";
import { WindowManager } from "~/main/window";
import type { CaseState } from "~/shared/case";
import { themeSchema } from "~/shared/theme";
import { assert } from "~/shared/utils";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class Application {
  private install?: Installation;
  private session?: SessionManager;
  private sessionDir?: string;
  private persist?: PersistedState;
  private caseManager?: CaseManager;
  private caseFlows?: CaseFlows;
  private windowManager?: WindowManager;
  private helpService?: HelpService;
  private quitConfirmed = false;

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
    app.on("before-quit", (event) => {
      this.onBeforeQuit(event);
    });
    app.on("will-quit", () => {
      this.onWillQuit();
    });
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

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
          this.onCaseChanged(state);
        },
        treeChanged: (document) => {
          broadcastIpcEvent("case", "treeChanged", document);
        },
      },
    );
    this.caseFlows = new CaseFlows(this.caseManager);

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

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  private onWindowAllClosed() {
    // On macOS the application conventionally stays alive without windows
    // and is reopened from the dock.
    if (process.platform !== "darwin") app.quit();
  }

  private onActivate() {
    this.windowManager?.controllers.main.open();
  }

  // Guard unsaved changes on quit: hold the quit, ask, then re-quit.
  private onBeforeQuit(event: Electron.Event) {
    if (this.quitConfirmed || this.caseFlows === undefined) return;
    const state = this.caseManager?.state() ?? null;
    if (state === null || !state.dirty) return;

    event.preventDefault();
    void this.caseFlows
      .confirmDiscard(this.windowManager?.controllers.main.window ?? null)
      .then((proceed) => {
        if (!proceed) return;
        this.quitConfirmed = true;
        app.quit();
      });
  }

  private onWillQuit() {
    this.session?.stop();
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  // React to case state changes: window chrome, menu, session scope.
  private onCaseChanged(state: CaseState) {
    const mainWindow = this.windowManager?.controllers.main;
    mainWindow?.setTitle(
      state === null ? "BlueTit" : `${state.name} — BlueTit`,
    );
    mainWindow?.setDocument(state?.dir ?? null, state?.dirty ?? false);

    this.updateAppMenu(state);
    this.resetSession(state);
    broadcastIpcEvent("case", "caseChanged", state);
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

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  private requireCase() {
    assert(this.caseManager !== undefined, "Case manager is not ready.");
    return this.caseManager;
  }

  private requireFlows() {
    assert(this.caseFlows !== undefined, "Case flows are not ready.");
    return this.caseFlows;
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
      theme: createThemeHandlers(),
      window: createWindowHandlers((event) => this.findWindowController(event)),
      case: createCaseHandlers(
        () => this.requireCase(),
        () => this.requireFlows(),
      ),
      session: createSessionHandlers(
        () => this.session,
        () => this.requireCase(),
      ),
      help: createHelpHandlers(() => this.requireHelp()),
    });
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  // Refresh the application menu (recents, item enablement).
  private updateAppMenu(state: CaseState) {
    const mainWindow = () =>
      this.windowManager?.controllers.main.window ?? null;
    updateApplicationMenu(
      {
        newCase: () => {
          runFromMenu(async () => this.requireFlows().newCase(mainWindow()));
        },
        openCase: () => {
          runFromMenu(async () => this.requireFlows().openCase(mainWindow()));
        },
        openRecentCase: (dir) => {
          runFromMenu(async () =>
            this.requireFlows().openRecent(mainWindow(), dir),
          );
        },
        saveCase: () => {
          runFromMenu(async () => this.requireCase().save());
        },
        closeCase: () => {
          runFromMenu(async () => this.requireFlows().close(mainWindow()));
        },
        openHelp: () => {
          this.windowManager?.controllers.help.open();
        },
      },
      { caseState: state, recents: this.caseManager?.recents() ?? [] },
    );
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
