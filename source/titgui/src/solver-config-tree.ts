/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { type PropertyRecord, type PropertyTree } from "~/solver-config";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export type PathPart = string | number;
export type Path = PathPart[];

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// These helpers are the editor's immutable tree operations. Keeping them out of
// React components makes path-based edits easier to test and reason about.

export function pathKey(path: Path): string {
  return path.join(".");
}

export function getValueAtPath(tree: PropertyTree, path: Path): PropertyTree {
  return path.reduce<PropertyTree>((current, part) => {
    if (typeof part === "number") return (current as PropertyTree[])[part];
    return (current as PropertyRecord)[part];
  }, tree);
}

export function setValueAtPath(
  tree: PropertyRecord,
  path: Path,
  nextValue: PropertyTree,
): PropertyRecord {
  return updateAtPath(tree, path, () => nextValue) as PropertyRecord;
}

export function removeValueAtPath(
  tree: PropertyRecord,
  path: Path,
): PropertyRecord {
  const parentPath = path.slice(0, -1);
  const key = path.at(-1);
  if (typeof key !== "number") return tree;
  return updateAtPath(tree, parentPath, (current) =>
    (current as PropertyTree[]).filter((_, index) => index !== key),
  ) as PropertyRecord;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

function updateAtPath(
  current: PropertyTree,
  path: Path,
  updater: (current: PropertyTree) => PropertyTree,
): PropertyTree {
  if (path.length === 0) return updater(current);

  const [head, ...rest] = path;

  if (typeof head === "number") {
    const array = [...(current as PropertyTree[])];
    array[head] = updateAtPath(array[head], rest, updater);
    return array;
  }

  return {
    ...(current as PropertyRecord),
    [head]: updateAtPath((current as PropertyRecord)[head], rest, updater),
  };
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
