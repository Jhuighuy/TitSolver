/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import fs from "node:fs";
import { createRequire } from "node:module";
import path from "node:path";
import process from "node:process";

import type { Storage } from "~/main/bindings/storage";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

interface NativeModule {
  openStorage(path: string): Promise<Storage>;
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

export type * from "~/main/bindings/storage";

export function openStorage(path: string) {
  return native.openStorage(path);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
