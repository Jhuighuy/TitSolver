/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import fs from "node:fs";
import { createRequire } from "node:module";
import path from "node:path";
import process from "node:process";

import type { Storage } from "~/bindings/storage";
import {
  type MaterializedCase,
  materializedCaseSchema,
  type SpecJson,
  specJsonSchema,
  type TreeJson,
  treeJsonSchema,
} from "~/shared/case";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// The case functions exchange JSON as text — trees are small, and text
// crosses the N-API boundary in one hop; the wrappers below parse it.
interface NativeModule {
  openStorage(path: string): Promise<Storage>;
  caseSpec(): Promise<string>;
  loadCaseTree(path: string): Promise<string>;
  saveCaseTree(path: string, treeText: string): Promise<void>;
  materializeCase(treeText: string): Promise<string>;
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

export async function openStorage(path: string) {
  return native.openStorage(path);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/** The WCSPH case specification. */
export async function caseSpec(): Promise<SpecJson> {
  return specJsonSchema.parse(JSON.parse(await native.caseSpec()));
}

/** Load a case tree from a `.yaml`/`.yml`/`.json` file. */
export async function loadCaseTree(path: string): Promise<TreeJson> {
  return treeJsonSchema.parse(JSON.parse(await native.loadCaseTree(path)));
}

/** Save a case tree to a `.yaml`/`.yml`/`.json` file. */
export async function saveCaseTree(
  path: string,
  tree: TreeJson,
): Promise<void> {
  await native.saveCaseTree(path, JSON.stringify(tree));
}

/** Materialize a case tree against the case specification. */
export async function materializeCase(
  tree: TreeJson,
): Promise<MaterializedCase> {
  return materializedCaseSchema.parse(
    JSON.parse(await native.materializeCase(JSON.stringify(tree))),
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
