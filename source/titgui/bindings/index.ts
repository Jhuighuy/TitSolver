/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import fs from "node:fs";
import { createRequire } from "node:module";
import path from "node:path";
import process from "node:process";

import type { PropSpec, PropTreeConstructor } from "~/bindings/properties";
import type { Storage } from "~/bindings/storage";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

interface NativeModule {
  PropTree: PropTreeConstructor;
  solverSpec(): PropSpec;
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

export type * from "~/bindings/storage";
export type * from "~/bindings/properties";

export const PropTree = native.PropTree;

export function solverSpec() {
  return native.solverSpec();
}

export async function openStorage(path: string) {
  return native.openStorage(path);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
