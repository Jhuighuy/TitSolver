/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { describe, expect, it } from "vitest";

import {
  treeDeleteAt,
  treeGetAt,
  treePathToString,
  treeSetAt,
} from "~/shared/case";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

describe("tree path helpers", () => {
  it("formats paths like validation issues", () => {
    expect(treePathToString(["fluid", "density"])).toBe("/fluid/density");
    expect(treePathToString([])).toBe("/");
  });

  it("gets nested values and misses gracefully", () => {
    const tree = { fluid: { density: 1000 } };
    expect(treeGetAt(tree, ["fluid", "density"])).toBe(1000);
    expect(treeGetAt(tree, ["fluid"])).toEqual({ density: 1000 });
    expect(treeGetAt(tree, [])).toBe(tree);
    expect(treeGetAt(tree, ["fluid", "viscosity"])).toBeUndefined();
    expect(treeGetAt(tree, ["bogus", "density"])).toBeUndefined();
    expect(treeGetAt(null, ["fluid"])).toBeUndefined();
  });

  it("sets values immutably, materializing intermediate maps", () => {
    const tree = { fluid: { density: 1000 } };
    const next = treeSetAt(tree, ["fluid", "viscosity"], 0.001);
    expect(next).toEqual({ fluid: { density: 1000, viscosity: 0.001 } });
    expect(tree).toEqual({ fluid: { density: 1000 } });

    expect(treeSetAt(null, ["a", "b"], 1)).toEqual({ a: { b: 1 } });
    expect(treeSetAt({ a: 5 }, ["a", "b"], 1)).toEqual({ a: { b: 1 } });
    expect(treeSetAt({ a: 5 }, [], null)).toBeNull();
  });

  it("deletes values immutably, pruning empty maps", () => {
    const tree = { fluid: { density: 1000 }, schema: 1 };
    expect(treeDeleteAt(tree, ["fluid", "density"])).toEqual({ schema: 1 });
    expect(tree).toEqual({ fluid: { density: 1000 }, schema: 1 });

    expect(treeDeleteAt(tree, ["schema"])).toEqual({
      fluid: { density: 1000 },
    });
    expect(treeDeleteAt(tree, ["bogus"])).toBe(tree);
    expect(treeDeleteAt(tree, ["fluid", "bogus"])).toEqual(tree);
    expect(treeDeleteAt(42, ["a"])).toBe(42);
  });
});

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
