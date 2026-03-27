/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { type TypedArray } from "three";

import { assert } from "~/shared/utils";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export function clamp(value: number, min: number, max: number): number {
  return Math.min(max, Math.max(min, value));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export function vectorMagnitude(values: TypedArray): number {
  let magnitudeSquared = 0;

  for (const v of values) {
    magnitudeSquared += v * v;
  }

  return Math.sqrt(magnitudeSquared);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export function matrixDeterminant(values: TypedArray): number {
  switch (values.length) {
    case 4: {
      const a00 = values[0];
      const a01 = values[1];
      const a10 = values[2];
      const a11 = values[3];
      return a00 * a11 - a01 * a10;
    }
    case 9: {
      const a00 = values[0];
      const a01 = values[1];
      const a02 = values[2];
      const a10 = values[3];
      const a11 = values[4];
      const a12 = values[5];
      const a20 = values[6];
      const a21 = values[7];
      const a22 = values[8];
      return (
        a00 * (a11 * a22 - a12 * a21) -
        a01 * (a10 * a22 - a12 * a20) +
        a02 * (a10 * a21 - a11 * a20)
      );
    }
    default:
      assert(false);
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
