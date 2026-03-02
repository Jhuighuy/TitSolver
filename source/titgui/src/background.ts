/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/** biome-ignore-all lint/complexity/noStaticOnlyClass: . */
/// <reference types="@electron-forge/plugin-vite/forge-vite-env" />
import child_process from "node:child_process";
import fs from "node:fs";
import path from "node:path";
import { app, BrowserWindow, dialog } from "electron";

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

class MainWindowManager {
  public static createWindow() {
    // Create the window.
    const window = new BrowserWindow({
      width: 1280,
      height: 800,
      minWidth: 1280,
      minHeight: 800,
      titleBarStyle: "hidden",
      webPreferences: { preload: path.join(__dirname, "preload.js") },
    });

    // Hide the window until content is loaded to avoid white flash.
    window.hide();
    void MainWindowManager.load(window).then(() => window.show());

    return window;
  }

  private static async load(window: BrowserWindow) {
    if (MAIN_WINDOW_VITE_DEV_SERVER_URL) {
      return window.loadURL(MAIN_WINDOW_VITE_DEV_SERVER_URL);
    } else {
      // .vite
      // |_ build <-- __dirname
      // |  |_ background.js
      // |_ renderer
      //    |_ `MAIN_WINDOW_VITE_NAME`
      //       |_ index.html
      void window.loadFile(
        path.join(
          __dirname,
          "..",
          "renderer",
          MAIN_WINDOW_VITE_NAME,
          "index.html",
        ),
      );
    }
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class BackendProcess {
  private process: child_process.ChildProcess | undefined;

  public constructor(private readonly installRoot: string) {}

  public start() {
    const backendExecutablePath = InstallRootResolver.resolveBackendPath(
      this.installRoot,
    );
    this.process = child_process.spawn(backendExecutablePath, {
      /** @todo Until we can select working directory from GUI. */
      cwd: path.resolve(this.installRoot, "../.."),
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
    const installRoot = await InstallRootResolver.resolveInstallRoot();
    if (installRoot === undefined) {
      dialog.showErrorBox(
        "BlueTit Solver",
        "BlueTit installation root not found.",
      );
      app.quit();
      return;
    }

    this.backend = new BackendProcess(installRoot);
    this.backend.start();

    MainWindowManager.createWindow();
  }

  private onWindowAllClosed() {
    app.quit();
  }

  private onBeforeQuit() {
    this.backend?.stop();
  }
}

new Application().run();

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
