/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { app, dialog } from "electron";
import child_process from "node:child_process";

import { Installation } from "~/main/installation";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
 * Server process manager.
 */
export class ServerManager {
  private process: child_process.ChildProcess | undefined;

  /**
   * Construct a server process manager.
   */
  public constructor(private readonly install: Installation) {}

  /**
   * Start the server.
   */
  public start(workDir: string) {
    // Spawn the server process.
    this.process = child_process.spawn(this.install.serverPath, {
      cwd: workDir,
    });

    // Handle errors.
    this.process.once("error", (error) => {
      if (this.process === undefined) return;
      dialog.showMessageBoxSync({
        type: "error",
        title: "Server encountered an error.",
        message: "Server encountered an error. Application will now quit.",
        detail: String(error),
      });
      app.exit(1);
    });

    // Handle exit.
    this.process.once("exit", (code, signal) => {
      if (this.process === undefined || code === 0) return;
      dialog.showMessageBoxSync({
        type: "error",
        title: "Server stopped unexpectedly.",
        message: "Server stopped unexpectedly. Application will now quit.",
        detail: `The server exited with ${code === null ? `signal ${signal}` : `code ${code}`}.`,
      });
      app.exit(1);
    });

    // Handle output.
    this.process.stdout?.on("data", (data: Buffer) => {
      console.log(data.toString());
    });
    this.process.stderr?.on("data", (data: Buffer) => {
      console.error(data.toString());
    });
  }

  /**
   * Stop the server.
   */
  public stop() {
    if (this.process === undefined) return;
    const process = this.process;
    this.process = undefined;
    process.kill("SIGTERM");
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
