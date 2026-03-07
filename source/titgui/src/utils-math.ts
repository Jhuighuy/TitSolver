/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { assert } from "~/utils";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export function clamp(value: number, min: number, max: number): number {
  return Math.min(max, Math.max(min, value));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export function vectorMagnitude(values: ArrayLike<number>): number {
  let magnitudeSquared = 0;

  for (let i = 0; i < values.length; i++) {
    const v = Number(values[i]);
    magnitudeSquared += v * v;
  }

  return Math.sqrt(magnitudeSquared);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export function matrixDeterminant(values: ArrayLike<number>): number {
  switch (values.length) {
    case 4: {
      const a00 = Number(values[0]);
      const a01 = Number(values[1]);
      const a10 = Number(values[2]);
      const a11 = Number(values[3]);
      return a00 * a11 - a01 * a10;
    }
    case 9: {
      const a00 = Number(values[0]);
      const a01 = Number(values[1]);
      const a02 = Number(values[2]);
      const a10 = Number(values[3]);
      const a11 = Number(values[4]);
      const a12 = Number(values[5]);
      const a20 = Number(values[6]);
      const a21 = Number(values[7]);
      const a22 = Number(values[8]);
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
