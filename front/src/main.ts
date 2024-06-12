/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { app, BrowserWindow } from "electron";
import { ChildProcess, spawn } from "child_process";

// This allows TypeScript to pick up the magic constants that's auto-generated
// by Forge's Webpack plugin that tells the Electron app where to look for the
// Webpack-bundled app code (depending on whether you're running in development
// or production).
declare const MAIN_WINDOW_WEBPACK_ENTRY: string;
declare const MAIN_WINDOW_PRELOAD_WEBPACK_ENTRY: string;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

let serverProcess: ChildProcess | null = null;

const createServer = (): void => {
  // Start the backend server.
  serverProcess = spawn(
    "/Users/jhuighuy/TitSolver/output/TIT_ROOT/bin/tit_server"
  );
  serverProcess.stdout.on("data", (data) => {
    console.log(`stdout: ${data}`);
  });
  serverProcess.stderr.on("data", (data) => {
    console.error(`stderr: ${data}`);
  });
  serverProcess.on("close", (code) => {
    console.log(`child process exited with code ${code}`);
  });
};

const killServer = (): void => {
  if (serverProcess) {
    serverProcess.kill();
  }
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

const createWindow = (): void => {
  // Create the browser window.
  const mainWindow = new BrowserWindow({
    height: 800,
    width: 1280,
    webPreferences: {
      preload: MAIN_WINDOW_PRELOAD_WEBPACK_ENTRY,
      webSecurity: false,
    },
  });

  // Load the index.html of the app.
  mainWindow.loadURL(MAIN_WINDOW_WEBPACK_ENTRY);
};

// This method will be called when Electron has finished initialization and is
// ready to create browser windows. Some APIs can only be used after this event
// occurs.
app.on("ready", () => {
  createServer();
  createWindow();
});

// Quit when all windows are closed, except on macOS. There, it's common for
// applications and their menu bar to stay active until the user quits
// explicitly with Cmd+Q.
app.on("window-all-closed", () => {
  if (process.platform !== "darwin") {
    app.quit();
  }
});

app.on("activate", () => {
  // On macOS it is common to re-create a window in the app when the dock icon
  // is clicked and there are no other windows open.
  if (BrowserWindow.getAllWindows().length === 0) {
    createWindow();
  }
});

app.on("quit", () => {
  // Ensure the server is killed when Electron quits.
  killServer();
});

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// In this file you can include the rest of your app's specific main process
// code. You can also put them in separate files and import them here.

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
