/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { Vector2 } from "three";
import { describe, expect, it } from "vitest";

import {
  clamp,
  matrixDeterminant,
  vectorMagnitude,
} from "~/renderer/common/utils-math";
import { Polygon2 } from "~/renderer/common/visual/polygon2";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

describe("math utilities", () => {
  it("clamps", () => {
    expect(clamp(5, 0, 10)).toBe(5);
    expect(clamp(-1, 0, 10)).toBe(0);
    expect(clamp(11, 0, 10)).toBe(10);
  });

  it("computes vector magnitudes", () => {
    expect(vectorMagnitude(new Float32Array([3, 4]))).toBe(5);
    expect(vectorMagnitude(new Float32Array([0, 0, 0]))).toBe(0);
  });

  it("computes 2×2 and 3×3 determinants", () => {
    expect(matrixDeterminant(new Float32Array([1, 2, 3, 4]))).toBe(-2);
    expect(
      matrixDeterminant(new Float32Array([2, 0, 0, 0, 3, 0, 0, 0, 4])),
    ).toBe(24);
    expect(
      matrixDeterminant(new Float32Array([1, 2, 3, 4, 5, 6, 7, 8, 9])),
    ).toBe(0);
  });
});

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

describe("Polygon2", () => {
  const square = new Polygon2([
    new Vector2(0, 0),
    new Vector2(10, 0),
    new Vector2(10, 10),
    new Vector2(0, 10),
  ]);

  it("contains interior points", () => {
    expect(square.containsPoint(new Vector2(5, 5))).toBe(true);
    expect(square.containsPoint(new Vector2(1, 9))).toBe(true);
  });

  it("excludes exterior points", () => {
    expect(square.containsPoint(new Vector2(-1, 5))).toBe(false);
    expect(square.containsPoint(new Vector2(11, 5))).toBe(false);
    expect(square.containsPoint(new Vector2(5, 12))).toBe(false);
  });

  it("handles concave polygons", () => {
    // A "C" shape: the notch on the right side is outside.
    const cShape = new Polygon2([
      new Vector2(0, 0),
      new Vector2(10, 0),
      new Vector2(10, 3),
      new Vector2(3, 3),
      new Vector2(3, 7),
      new Vector2(10, 7),
      new Vector2(10, 10),
      new Vector2(0, 10),
    ]);
    expect(cShape.containsPoint(new Vector2(1, 5))).toBe(true);
    expect(cShape.containsPoint(new Vector2(7, 5))).toBe(false);
    expect(cShape.containsPoint(new Vector2(7, 1))).toBe(true);
  });

  it("clones its points", () => {
    const point = new Vector2(0, 0);
    const polygon = new Polygon2([point]);
    point.set(100, 100);
    expect(polygon.points[0].x).toBe(0);
  });
});

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
