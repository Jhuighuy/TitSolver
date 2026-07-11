/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import fs from "node:fs";
import os from "node:os";
import path from "node:path";

import {
  _electron as electron,
  type ElectronApplication,
  expect,
  type Page,
  test,
} from "@playwright/test";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Smoke test: the app launches against a fresh case directory, the
// workspace renders, and the main flows respond. This is the guard against
// the "silent dead process on startup" class of regressions.

let app: ElectronApplication;
let window: Page;
let scratchDir: string;
const consoleErrors: string[] = [];

test.beforeAll(async () => {
  // A fresh case directory and an isolated user-data directory, so the
  // test never touches (or depends on) real application state.
  scratchDir = fs.mkdtempSync(path.join(os.tmpdir(), "titgui-e2e-"));
  const caseDir = path.join(scratchDir, "smoke-case");
  fs.mkdirSync(caseDir);
  fs.writeFileSync(
    path.join(caseDir, "case.yaml"),
    "schema: 1\nsimulation:\n  title: Smoke Case\n",
    "utf8",
  );

  app = await electron.launch({
    args: ["."],
    env: {
      ...process.env,
      // Resolve the installation like a packaged app would, but against the
      // build output of this repository.
      TIT_ROOT: path.resolve(__dirname, "../../../output/TIT_ROOT"),
      TITGUI_USER_DATA: path.join(scratchDir, "user-data"),
      TITGUI_CASE: caseDir,
    },
  });

  window = await app.firstWindow();
  window.on("console", (message) => {
    if (message.type() === "error") consoleErrors.push(message.text());
  });
  await window.waitForLoadState("domcontentloaded");
});

test.afterAll(async () => {
  await app.close();
  fs.rmSync(scratchDir, { recursive: true, force: true });
});

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

test("launches into the welcome tab", async () => {
  await expect(window.getByRole("tab", { name: "Welcome" })).toBeVisible();
  await expect(window.getByText("BlueTit Solver").last()).toBeVisible();
  await expect(window.getByRole("button", { name: "New Case…" })).toBeVisible();
});

test("shows the opened case in the recents list", async () => {
  await expect(window.getByText("smoke-case").first()).toBeVisible();
});

test("switches to the viewport tab with controls and timeline", async () => {
  await window.getByRole("tab", { name: "Viewport" }).click();
  await expect(window.getByText("Interact")).toBeVisible();
  await expect(window.getByText("Display")).toBeVisible();
  await expect(window.getByRole("slider")).toBeVisible();
});

test("edits the case through the setup pane", async () => {
  await window.getByRole("button", { name: "Setup" }).click();

  // The materialized document is shown: the authored title, then the
  // defaulted numeric fields (End Time, Gravity, …) in spec order.
  const fields = window.getByRole("textbox");
  await expect(fields.nth(0)).toHaveValue("Smoke Case");
  await expect(fields.nth(1)).toHaveValue("10");

  // Commit an edit and expect the explicit-value reset control to appear.
  const gravity = fields.nth(2);
  await expect(gravity).toHaveValue("9.81");
  await gravity.fill("9.9");
  await gravity.blur();
  await expect(
    window.getByRole("button", { name: "Reset Gravity to default" }),
  ).toBeVisible();
});

test("logged no console errors", () => {
  expect(consoleErrors).toEqual([]);
});

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
