/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { app, dialog } from "electron";
import path from "node:path";
import process from "node:process";

import { canExecute } from "~/main/utils";
import { assert } from "~/shared/utils";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
 * Accessor for package installation information, such as paths to bundled
 * binaries and resources.
 */
export class Installation {
  private constructor(
    /** The path to the installation root. */
    public readonly rootPath: string,
  ) {}

  /**
   * The path to the package binary directory.
   */
  public get binPath() {
    return path.join(this.rootPath, "bin");
  }

  /**
   * The path to the solver binary.
   */
  public get solverPath() {
    return path.join(this.binPath, "titwcsph");
  }

  /**
   * The path to the manual directory.
   */
  public get manualPath() {
    return path.join(this.rootPath, "manual");
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /**
   * Resolve the package installation paths across various scenarios.
   */
  public static resolve() {
    let installation: Installation | undefined;

    // Keep the development path first so local GUI runs can find package root
    // without prompting.
    installation = Installation.tryResolveFromDevelopment();
    if (installation !== undefined) return installation;

    // Environment override comes next.
    installation = Installation.tryResolveFromEnvironment();
    if (installation !== undefined) return installation;

    // Fall back to builtin installation root.
    installation = Installation.tryResolveBuiltin();
    if (installation !== undefined) return installation;

    // Prompt user for installation folder last.
    installation = Installation.promptUserForRootPath();
    if (installation !== undefined) return installation;

    // If all else fails, error out.
    dialog.showErrorBox(
      "BlueTit installation not found.",
      "Unable to find BlueTit installation.",
    );
    app.exit(1);
    assert(false);
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  // Try to resolve the installation root from provided path.
  // Returns `undefined` if the installation root is invalid.
  private static tryResolveFromPath(rootPath: string) {
    const installation = new Installation(rootPath);
    if (canExecute(installation.solverPath)) return installation;
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  // Try to resolve the installation root based on the environment variable.
  // Returns `undefined` if the environment variable is not set, or if the
  // installation root is invalid (warning is issued in this case).
  private static tryResolveFromEnvironment() {
    const TIT_ROOT = "TIT_ROOT";
    const rootPath = process.env[TIT_ROOT]?.trim() ?? "";
    if (rootPath !== "") {
      const installation = Installation.tryResolveFromPath(rootPath);
      if (installation === undefined) {
        dialog.showMessageBoxSync({
          type: "warning",
          title: "Environment variable invalid",
          message: `Environment variable '${TIT_ROOT}' is invalid.`,
          detail: [
            `Path '${rootPath}' does not contain a valid BlueTit installation.`,
            `Either unset '${TIT_ROOT}' or set it to a valid value.`,
          ].join("\n"),
        });
      }
      return installation;
    }
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  // Try to resolve the development installation root based on the environment.
  // Returns `undefined` if no development environment is detected, errors
  // if the installation root is invalid.
  private static tryResolveFromDevelopment() {
    if (process.env.NODE_ENV === "development") {
      // <project>
      // |_ source
      // |  |_ titgui
      // |     |_ .vite
      // |        |_ build <-- __dirname
      // |           |_ background.js
      // |_ output
      //    |_ TIT_ROOT
      //       |_ <install>
      const rootPath = path.resolve(
        __dirname,
        "..",
        "..",
        "..",
        "..",
        "output",
        "TIT_ROOT",
      );

      const installation = Installation.tryResolveFromPath(rootPath);
      if (installation === undefined) {
        dialog.showErrorBox(
          "Development environment invalid",
          `Installation root not found at '${rootPath}'.`,
        );
        app.exit(1);
      }
      return installation;
    }
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  // Try to resolve the built-in installation root based on the executable path.
  // Returns `undefined` if the installation root is invalid.
  private static tryResolveBuiltin() {
    const execDir = path.dirname(process.execPath);

    // macOS.
    if (process.platform === "darwin") {
      // <install>
      // |_ <bundle>
      //    |_ Contents
      //       |_ MacOS <-- execDir
      //          |_ <executable>
      const rootPath = path.resolve(execDir, "..", "..", "..");
      return Installation.tryResolveFromPath(rootPath);
    }

    // Linux.
    if (process.platform === "linux") {
      // <install>
      // |_ lib
      //    |_ gui <-- execDir
      //       |_ <executable>
      const rootPath = path.resolve(execDir, "..", "..");
      return Installation.tryResolveFromPath(rootPath);
    }

    // Unknown platform?
    assert(false);
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  private static promptUserForRootPath() {
    for (let retry = 0; retry < 3; retry++) {
      // Prompt user for install root.
      const rootPaths = dialog.showOpenDialogSync({
        title: "Select BlueTit installation folder.",
        message: "Choose the folder that contains BlueTit installation.",
        buttonLabel: "Use This Folder",
        properties: ["openDirectory"],
      });
      if (rootPaths === undefined || rootPaths.length === 0) break;

      // Validate the selected folder.
      const rootPath = rootPaths[0];
      const installation = Installation.tryResolveFromPath(rootPath);
      if (installation !== undefined) return installation;

      // Show error and retry.
      const result = dialog.showMessageBoxSync({
        type: "error",
        title: "Invalid installation folder.",
        message: [
          "The selected folder is not a valid BlueTit installation.",
          "Please select the folder that contains the BlueTit executable.",
        ].join("\n"),
        buttons: ["Try Again", "Quit"],
        defaultId: 0,
        cancelId: 1,
      });
      if (result === 1) break;
    }
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
