/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/** biome-ignore-all lint/complexity/noStaticOnlyClass: . */
/// <reference types="@electron-forge/plugin-vite/forge-vite-env" />
import child_process from "node:child_process";
import fs from "node:fs";
import path from "node:path";
import { pathToFileURL } from "node:url";
import { app, BrowserWindow, dialog, ipcMain, shell } from "electron";
import { z } from "zod";

import {
  HELP_WINDOW_HEIGHT_KEY,
  HELP_WINDOW_MAXIMIZED_KEY,
  HELP_WINDOW_WIDTH_KEY,
  HELP_WINDOW_X_KEY,
  HELP_WINDOW_Y_KEY,
  IPC_FULL_SCREEN_CHANGED,
  IPC_HELP_OPEN,
  IPC_IS_FULL_SCREEN,
  IPC_PERSIST_GET,
  IPC_PERSIST_SET,
  WINDOW_FULLSCREEN_KEY,
  WINDOW_HEIGHT_KEY,
  WINDOW_MAXIMIZED_KEY,
  WINDOW_WIDTH_KEY,
  WINDOW_X_KEY,
  WINDOW_Y_KEY,
} from "~/constants";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class InstallRootResolver {
  public static resolveBackendPath(installRoot: string) {
    return path.join(installRoot, "bin", "titback");
  }

  public static async resolveInstallRoot() {
    // Keep this this first.
    if (MAIN_WINDOW_VITE_DEV_SERVER_URL) {
      const devInstallRoot = InstallRootResolver.resolveDevInstallRoot();
      if (InstallRootResolver.isValidInstallRoot(devInstallRoot)) {
        return devInstallRoot;
      }
    }

    // Check environment variable next, it overrides default paths.
    const installRootFromEnv =
      InstallRootResolver.resolveInstallRootFromEnvironment();
    if (InstallRootResolver.isValidInstallRoot(installRootFromEnv)) {
      return installRootFromEnv;
    }

    // Check default paths last.
    const defaultInstallRoot = InstallRootResolver.resolveDefaultInstallRoot();
    if (InstallRootResolver.isValidInstallRoot(defaultInstallRoot)) {
      return defaultInstallRoot;
    }

    // If all else fails, prompt the user.
    return InstallRootResolver.promptUserForInstallRoot();
  }

  private static resolveDevInstallRoot() {
    // <projectRoot>
    // |_ source
    // |  |_ titgui
    // |     |_ .vite
    // |        |_ build <-- __dirname
    // |           |_ background.js
    // |_ output
    //    |_ <install>
    return path.resolve(
      __dirname,
      "..",
      "..",
      "..",
      "..",
      "output",
      "TIT_ROOT",
    );
  }

  private static resolveInstallRootFromEnvironment() {
    const envRoot = process.env.TIT_ROOT;
    if (envRoot === undefined || envRoot.trim() === "") {
      return undefined;
    }
    return path.resolve(envRoot);
  }

  private static resolveDefaultInstallRoot() {
    const execDir = path.dirname(process.execPath);

    // MacOS:
    // <install>
    // |_ <bundle>
    //    |_ Contents
    //       |_ MacOS <-- execDir
    //          |_ <executable>
    if (process.platform === "darwin") {
      return path.resolve(execDir, "..", "..", "..");
    }

    // Linux:
    // <install>
    // |_ lib
    //    |_ gui <-- execDir
    //       |_ <executable>
    if (process.platform === "linux") {
      return path.resolve(execDir, "..", "..");
    }

    // Unknown platform?
    return undefined;
  }

  private static async promptUserForInstallRoot() {
    for (let retry = 0; retry < 3; retry++) {
      // Prompt user for install root.
      const selected = await dialog.showOpenDialog({
        title: "Select BlueTit installation root",
        message: "Choose the folder that contains BlueTit installation",
        buttonLabel: "Use This Folder",
        properties: ["openDirectory"],
      });
      if (selected.canceled || selected.filePaths.length === 0) {
        break;
      }

      // Validate the selected folder.
      const installRoot = selected.filePaths[0];
      if (InstallRootResolver.isValidInstallRoot(installRoot)) {
        return installRoot;
      }

      // Show error and retry.
      const result = await dialog.showMessageBox({
        type: "error",
        title: "Invalid installation root",
        message: [
          "The selected folder is not a valid BlueTit installation.",
          "Please select the folder that contains the BlueTit executable.",
        ].join("\n"),
        buttons: ["Try Again", "Quit"],
        defaultId: 0,
        cancelId: 1,
      });
      if (result.response === 1) break;
    }
    return undefined;
  }

  private static isValidInstallRoot(installRoot?: string) {
    return (
      installRoot !== undefined &&
      InstallRootResolver.canExecute(
        InstallRootResolver.resolveBackendPath(installRoot),
      )
    );
  }

  private static canExecute(pathToExecutable: string) {
    try {
      fs.accessSync(pathToExecutable, fs.constants.X_OK);
      return true;
    } catch {
      return false;
    }
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class PersistedState {
  private constructor(
    private readonly path: string,
    private readonly data: z.infer<typeof persistedStateSchema>,
  ) {}

  public static load(path: string) {
    return new PersistedState(
      path,
      (() => {
        let content: string;
        try {
          content = fs.readFileSync(path, "utf8");
        } catch {
          return {}; // File does not exist, do not issue warning.
        }

        try {
          const parsed = persistedStateSchema.safeParse(JSON.parse(content));
          if (parsed.success) return parsed.data;
          console.warn(
            `Invalid persisted state format in ${path}. Using empty state.`,
            `Error: ${parsed.error.message}`,
          );
        } catch (error) {
          console.warn(
            `Failed to parse persisted state from ${path}. Using empty state.`,
            `Error: ${String(error)}`,
          );
        }
        return {};
      })(),
    );
  }

  public get<T>(key: string, schema: z.ZodType<T>): T | undefined;
  public get<T>(key: string, schema: z.ZodType<T>, fallbackValue: T): T;
  public get<T>(
    key: string,
    schema: z.ZodType<T>,
    fallbackValue?: T,
  ): T | undefined {
    // Query the value. If it does not exist, silently return the fallback.
    const persistedValue = this.data[key];
    if (persistedValue === undefined) return fallbackValue;

    // Parse the value. If it is invalid, issue a warning and return the fallback.
    const parsed = schema.safeParse(persistedValue);
    if (!parsed.success) {
      console.warn(
        `Ignoring invalid persisted value for '${key}'.`,
        `Error: ${parsed.error.message}`,
      );
      return fallbackValue;
    }

    // Return the parsed value.
    return parsed.data;
  }

  public set(key: string, value: unknown) {
    this.data[key] = value;
  }

  public save() {
    try {
      fs.mkdirSync(path.dirname(this.path), { recursive: true });
      fs.writeFileSync(
        this.path,
        JSON.stringify(this.data, undefined, 2),
        "utf8",
      );
    } catch (error) {
      console.warn(
        `Failed to save persisted state.`,
        `Error: ${String(error)}`,
      );
    }
  }
}

const persistedStateSchema = z.record(z.string(), z.unknown());

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

const WINDOW_MIN_WIDTH = 1280;
const WINDOW_MIN_HEIGHT = 800;
const HELP_WINDOW_MIN_WIDTH = 960;
const HELP_WINDOW_MIN_HEIGHT = 720;
const HELP_WINDOW_TITLE = "BlueTit Manual";

function getFrontendEntryUrl(searchParams?: Record<string, string>) {
  if (MAIN_WINDOW_VITE_DEV_SERVER_URL) {
    const url = new URL(MAIN_WINDOW_VITE_DEV_SERVER_URL);
    for (const [key, value] of Object.entries(searchParams ?? {})) {
      url.searchParams.set(key, value);
    }
    return url.toString();
  }

  const url = pathToFileURL(
    path.join(__dirname, "..", "renderer", MAIN_WINDOW_VITE_NAME, "index.html"),
  );
  for (const [key, value] of Object.entries(searchParams ?? {})) {
    url.searchParams.set(key, value);
  }
  return url.toString();
}

function getManualRootPath(installRoot: string) {
  return path.join(installRoot, "manual");
}

function getManualUrl(installRoot: string) {
  return pathToFileURL(
    path.join(getManualRootPath(installRoot), "index.html"),
  ).toString();
}

function isManualUrl(url: string, installRoot: string) {
  try {
    const parsed = new URL(url);
    if (parsed.protocol !== "file:") return false;

    const manualRootPath = `${getManualRootPath(installRoot)}${path.sep}`;
    const filePath = decodeURIComponent(parsed.pathname);
    return filePath.startsWith(manualRootPath);
  } catch {
    return false;
  }
}

class MainWindowManager {
  public static createWindow(persist: PersistedState) {
    // Read persisted window state.
    const x = persist.get(WINDOW_X_KEY, z.int());
    const y = persist.get(WINDOW_Y_KEY, z.int());
    const width = persist.get(
      WINDOW_WIDTH_KEY,
      z.int().min(WINDOW_MIN_WIDTH),
      WINDOW_MIN_WIDTH,
    );
    const height = persist.get(
      WINDOW_HEIGHT_KEY,
      z.int().min(WINDOW_MIN_HEIGHT),
      WINDOW_MIN_HEIGHT,
    );
    const isMaximized = persist.get(WINDOW_MAXIMIZED_KEY, z.boolean(), false);
    const isFullScreen = persist.get(WINDOW_FULLSCREEN_KEY, z.boolean(), false);

    // Create the window.
    const window = new BrowserWindow({
      x,
      y,
      width,
      height,
      minWidth: WINDOW_MIN_WIDTH,
      minHeight: WINDOW_MIN_HEIGHT,
      titleBarStyle: "hidden",
      webPreferences: { preload: path.join(__dirname, "preload.js") },
    });

    // Attach listeners.
    MainWindowManager.attachFullScreenListeners(window);
    MainWindowManager.attachPersistenceListener(window, persist);

    // Hide the window until content is loaded to avoid white flash.
    window.hide();
    void MainWindowManager.load(window).then(() => {
      window.show();
      if (isMaximized) window.maximize();
      if (isFullScreen) window.setFullScreen(true);
    });

    return window;
  }

  public static attachFullScreenListeners(window: BrowserWindow) {
    const notify = () => {
      window.webContents.send(IPC_FULL_SCREEN_CHANGED, window.isFullScreen());
    };
    window.on("enter-full-screen", notify);
    window.on("leave-full-screen", notify);
    window.once("ready-to-show", notify);
  }

  private static attachPersistenceListener(
    window: BrowserWindow,
    persistedState: PersistedState,
  ) {
    window.on("close", () => {
      const bounds =
        window.isMaximized() || window.isFullScreen()
          ? window.getNormalBounds()
          : window.getBounds();

      persistedState.set(WINDOW_X_KEY, bounds.x);
      persistedState.set(WINDOW_Y_KEY, bounds.y);
      persistedState.set(WINDOW_WIDTH_KEY, bounds.width);
      persistedState.set(WINDOW_HEIGHT_KEY, bounds.height);
      persistedState.set(WINDOW_MAXIMIZED_KEY, window.isMaximized());
      persistedState.set(WINDOW_FULLSCREEN_KEY, window.isFullScreen());

      persistedState.save();
    });
  }

  private static async load(window: BrowserWindow) {
    return window.loadURL(getFrontendEntryUrl());
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class HelpWindowManager {
  private static window: BrowserWindow | undefined;

  public static open(installRoot: string, persist: PersistedState) {
    if (
      HelpWindowManager.window !== undefined &&
      !HelpWindowManager.window.isDestroyed()
    ) {
      HelpWindowManager.window.focus();
      return HelpWindowManager.window;
    }

    const x = persist.get(HELP_WINDOW_X_KEY, z.int());
    const y = persist.get(HELP_WINDOW_Y_KEY, z.int());
    const width = persist.get(
      HELP_WINDOW_WIDTH_KEY,
      z.int().min(HELP_WINDOW_MIN_WIDTH),
      1200,
    );
    const height = persist.get(
      HELP_WINDOW_HEIGHT_KEY,
      z.int().min(HELP_WINDOW_MIN_HEIGHT),
      900,
    );
    const isMaximized = persist.get(
      HELP_WINDOW_MAXIMIZED_KEY,
      z.boolean(),
      false,
    );

    const window = new BrowserWindow({
      x,
      y,
      width,
      height,
      minWidth: HELP_WINDOW_MIN_WIDTH,
      minHeight: HELP_WINDOW_MIN_HEIGHT,
      title: HELP_WINDOW_TITLE,
      titleBarStyle: "hidden",
      autoHideMenuBar: true,
      webPreferences: {
        preload: path.join(__dirname, "preload.js"),
        webviewTag: true,
      },
    });

    MainWindowManager.attachFullScreenListeners(window);
    HelpWindowManager.attachPersistenceListener(window, persist);
    HelpWindowManager.attachLifecycleListeners(window);

    window.hide();
    void HelpWindowManager.load(window, installRoot).then(() => {
      window.show();
      if (isMaximized) window.maximize();
    });

    HelpWindowManager.window = window;
    return window;
  }

  private static attachPersistenceListener(
    window: BrowserWindow,
    persistedState: PersistedState,
  ) {
    window.on("close", () => {
      const bounds = window.isMaximized()
        ? window.getNormalBounds()
        : window.getBounds();

      persistedState.set(HELP_WINDOW_X_KEY, bounds.x);
      persistedState.set(HELP_WINDOW_Y_KEY, bounds.y);
      persistedState.set(HELP_WINDOW_WIDTH_KEY, bounds.width);
      persistedState.set(HELP_WINDOW_HEIGHT_KEY, bounds.height);
      persistedState.set(HELP_WINDOW_MAXIMIZED_KEY, window.isMaximized());
      persistedState.save();
    });
  }

  private static attachLifecycleListeners(window: BrowserWindow) {
    window.on("closed", () => {
      if (HelpWindowManager.window === window) {
        HelpWindowManager.window = undefined;
      }
    });
  }

  private static async load(window: BrowserWindow, installRoot: string) {
    return window.loadURL(
      getFrontendEntryUrl({
        window: "help",
        manualUrl: getManualUrl(installRoot),
      }),
    );
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class BackendProcess {
  private process: child_process.ChildProcess | undefined;

  public constructor(
    private readonly installRoot: string,
    private readonly workDir: string,
  ) {}

  public start() {
    const backendExecutablePath = InstallRootResolver.resolveBackendPath(
      this.installRoot,
    );
    this.process = child_process.spawn(backendExecutablePath, {
      cwd: this.workDir,
    });

    this.process.once("exit", () => {
      this.process = undefined;
    });

    this.process.stdout?.on("data", (data: Buffer) => {
      console.log(data.toString());
    });
    this.process.stderr?.on("data", (data: Buffer) => {
      console.error(data.toString());
    });
  }

  public stop() {
    if (!this.process || this.process.killed) return;
    this.process.kill("SIGTERM");
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class Application {
  private backend: BackendProcess | undefined;
  private installRoot: string | undefined;
  private persistedState: PersistedState | undefined;

  public run() {
    app.on("ready", () => {
      void this.onReady();
    });
    app.on("window-all-closed", () => {
      this.onWindowAllClosed();
    });
    app.on("before-quit", () => {
      this.onBeforeQuit();
    });
  }

  private async onReady() {
    // Find the installation root.
    const installRoot = await InstallRootResolver.resolveInstallRoot();
    if (installRoot === undefined) {
      dialog.showErrorBox(
        "BlueTit Solver",
        "BlueTit installation root not found.",
      );
      app.quit();
      return;
    }

    /** @todo Until we can select working directory from GUI. */
    const workDir = path.resolve(installRoot, "../..");
    this.installRoot = installRoot;

    // Start the backend.
    this.backend = new BackendProcess(installRoot, workDir);
    this.backend.start();

    // Load persisted state and create the main window.
    this.persistedState = PersistedState.load(
      path.join(workDir, "persisted-state.json"),
    );
    this.registerIpcHandlers();
    this.registerWebContentsHandlers();
    MainWindowManager.createWindow(this.persistedState);
  }

  private onWindowAllClosed() {
    app.quit();
  }

  private onBeforeQuit() {
    this.backend?.stop();
  }

  private registerIpcHandlers() {
    ipcMain.removeHandler(IPC_IS_FULL_SCREEN);
    ipcMain.handle(IPC_IS_FULL_SCREEN, (event) => {
      const window = BrowserWindow.fromWebContents(event.sender);
      return window?.isFullScreen() ?? false;
    });

    ipcMain.removeHandler(IPC_PERSIST_GET);
    ipcMain.handle(IPC_PERSIST_GET, (_event, key: unknown) => {
      if (typeof key !== "string" || this.persistedState === undefined) {
        return undefined;
      }
      return this.persistedState.get(key, z.unknown());
    });

    ipcMain.removeHandler(IPC_PERSIST_SET);
    ipcMain.handle(IPC_PERSIST_SET, (_event, key: unknown, value: unknown) => {
      if (typeof key !== "string" || this.persistedState === undefined) {
        return;
      }
      this.persistedState.set(key, value);
    });

    ipcMain.removeHandler(IPC_HELP_OPEN);
    ipcMain.handle(IPC_HELP_OPEN, () => {
      if (this.persistedState === undefined) return;
      if (this.installRoot === undefined) return;
      HelpWindowManager.open(this.installRoot, this.persistedState);
    });
  }

  private registerWebContentsHandlers() {
    app.on("web-contents-created", (_event, contents) => {
      if (contents.getType() !== "webview") return;

      contents.setWindowOpenHandler(({ url }) => {
        if (this.installRoot !== undefined && isManualUrl(url, this.installRoot)) {
          queueMicrotask(() => {
            void contents.loadURL(url);
          });
          return { action: "deny" };
        }
        void shell.openExternal(url);
        return { action: "deny" };
      });

      contents.on("will-navigate", (event, url) => {
        if (this.installRoot !== undefined && isManualUrl(url, this.installRoot)) {
          return;
        }
        event.preventDefault();
        void shell.openExternal(url);
      });
    });
  }
}

new Application().run();

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
