/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import fs from "node:fs";
import os from "node:os";
import path from "node:path";

import { app, dialog } from "electron";
import { afterEach, beforeEach, describe, expect, it, vi } from "vitest";

import { Installation } from "~/main/installation";

vi.mock("electron");

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

let tempDir: string;
let validRoot: string;
let invalidRoot: string;

beforeEach(() => {
  vi.clearAllMocks();

  tempDir = fs.mkdtempSync(path.join(os.tmpdir(), "titgui-install-"));

  // A valid installation has an executable solver under `bin/`.
  validRoot = path.join(tempDir, "valid");
  fs.mkdirSync(path.join(validRoot, "bin"), { recursive: true });
  fs.writeFileSync(path.join(validRoot, "bin", "titwcsph"), "#!/bin/sh\n", {
    mode: 0o755,
  });

  invalidRoot = path.join(tempDir, "invalid");
  fs.mkdirSync(invalidRoot);
});

afterEach(() => {
  vi.unstubAllEnvs();
  fs.rmSync(tempDir, { recursive: true, force: true });
});

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

describe("Installation", () => {
  it("derives all paths from the root", () => {
    vi.stubEnv("TIT_ROOT", validRoot);
    const installation = Installation.resolve();

    expect(installation.rootPath).toBe(validRoot);
    expect(installation.binPath).toBe(path.join(validRoot, "bin"));
    expect(installation.solverPath).toBe(
      path.join(validRoot, "bin", "titwcsph"),
    );
    expect(installation.manualPath).toBe(path.join(validRoot, "manual"));
  });

  it("warns about an invalid TIT_ROOT and falls through", () => {
    vi.stubEnv("TIT_ROOT", invalidRoot);
    // With no other candidates, resolution ends in a fatal error.
    vi.mocked(dialog.showOpenDialogSync).mockReturnValue(undefined);

    expect(() => Installation.resolve()).toThrow();
    expect(dialog.showMessageBoxSync).toHaveBeenCalledWith(
      expect.objectContaining({ type: "warning" }),
    );
    expect(dialog.showErrorBox).toHaveBeenCalled();
    expect(app.exit).toHaveBeenCalledWith(1);
  });

  it("accepts a valid folder from the prompt", () => {
    vi.mocked(dialog.showOpenDialogSync).mockReturnValue([validRoot]);
    expect(Installation.resolve().rootPath).toBe(validRoot);
  });

  it("retries the prompt after an invalid selection", () => {
    vi.mocked(dialog.showOpenDialogSync)
      .mockReturnValueOnce([invalidRoot])
      .mockReturnValueOnce([validRoot]);
    // "Try Again" on the invalid-folder error.
    vi.mocked(dialog.showMessageBoxSync).mockReturnValue(0);

    expect(Installation.resolve().rootPath).toBe(validRoot);
    expect(dialog.showOpenDialogSync).toHaveBeenCalledTimes(2);
  });

  it("errors out when the prompt is dismissed", () => {
    vi.mocked(dialog.showOpenDialogSync).mockReturnValue(undefined);

    expect(() => Installation.resolve()).toThrow();
    expect(dialog.showErrorBox).toHaveBeenCalledWith(
      "BlueTit installation not found.",
      expect.any(String),
    );
    expect(app.exit).toHaveBeenCalledWith(1);
  });

  it("quits the prompt loop on user request", () => {
    vi.mocked(dialog.showOpenDialogSync).mockReturnValue([invalidRoot]);
    // "Quit" on the invalid-folder error.
    vi.mocked(dialog.showMessageBoxSync).mockReturnValue(1);

    expect(() => Installation.resolve()).toThrow();
    expect(dialog.showOpenDialogSync).toHaveBeenCalledTimes(1);
  });
});

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
