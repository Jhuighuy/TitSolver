/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import fs from "node:fs";
import { createRequire } from "node:module";
import path from "node:path";
import process from "node:process";

import type { Run } from "~/bindings/run";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

interface NativeModule {
  openRun(path: string): Promise<Run>;
}

const native = (() => {
  const localRequire = createRequire(__filename);
  const moduleFileName = "titgui-bindings.node";

  const developmentPath = path.join(
    __dirname,
    "..",
    "..",
    "native",
    moduleFileName,
  );
  if (fs.existsSync(developmentPath)) {
    return localRequire(developmentPath) as NativeModule;
  }

  const packagedPath = path.join(
    process.resourcesPath,
    "native",
    moduleFileName,
  );
  return localRequire(packagedPath) as NativeModule;
})();

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export type * from "~/bindings/run";

export async function openRun(path: string) {
  return native.openRun(path);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
