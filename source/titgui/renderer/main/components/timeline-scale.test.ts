/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { describe, expect, it } from "vitest";

import {
  computeFrameTicks,
  niceStep,
} from "~/renderer/main/components/timeline-scale";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

describe("niceStep", () => {
  it("returns 1/2/5×10ⁿ steps", () => {
    expect(niceStep(0.5)).toBe(1);
    expect(niceStep(1)).toBe(1);
    expect(niceStep(1.5)).toBe(2);
    expect(niceStep(2)).toBe(2);
    expect(niceStep(3)).toBe(5);
    expect(niceStep(7)).toBe(10);
    expect(niceStep(15)).toBe(20);
    expect(niceStep(60)).toBe(100);
    expect(niceStep(400)).toBe(500);
    expect(niceStep(1234)).toBe(2000);
  });
});

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

describe("computeFrameTicks", () => {
  it("labels every 5th frame at 10px per frame", () => {
    // Labels need ≥ 48px → step 5; minors need ≥ 6px → every frame.
    const { labeledTicks, minorTicks } = computeFrameTicks(11, 10);
    expect(labeledTicks).toEqual([0, 5, 10]);
    expect(minorTicks).toEqual([1, 2, 3, 4, 6, 7, 8, 9]);
  });

  it("labels every frame when there is room", () => {
    const { labeledTicks, minorTicks } = computeFrameTicks(4, 100);
    expect(labeledTicks).toEqual([0, 1, 2, 3]);
    expect(minorTicks).toEqual([]);
  });

  it("drops minor ticks that would be denser than their spacing", () => {
    // 1px per frame: labels every 50, minors every 10 — never per frame.
    const { labeledTicks, minorTicks } = computeFrameTicks(101, 1);
    expect(labeledTicks).toEqual([0, 50, 100]);
    expect(minorTicks).toEqual([10, 20, 30, 40, 60, 70, 80, 90]);
  });

  it("keeps tick counts bounded regardless of the frame count", () => {
    const { labeledTicks, minorTicks } = computeFrameTicks(1_000_000, 0.001);
    // 1000px worth of scale: minors ≥ 6px, labels ≥ 48px apart.
    expect(minorTicks.length + labeledTicks.length).toBeLessThan(200);
  });

  it("handles empty and single-frame series", () => {
    expect(computeFrameTicks(0, 100)).toEqual({
      labeledTicks: [],
      minorTicks: [],
    });
    expect(computeFrameTicks(1, 500)).toEqual({
      labeledTicks: [0],
      minorTicks: [],
    });
  });
});

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
