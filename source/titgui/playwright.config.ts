/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { defineConfig } from "@playwright/test";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// End-to-end tests drive the real Electron app, so the production bundles
// must exist: run `npm run build` before `npm run e2e`. The CTest
// registration (`titgui/e2e[long]`) takes care of that ordering.
export default defineConfig({
  testDir: "./e2e",
  timeout: 60_000,
  // One Electron instance at a time.
  workers: 1,
  fullyParallel: false,
  reporter: [["list"]],
  outputDir: "./test-results",
});

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
