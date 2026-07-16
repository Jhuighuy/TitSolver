/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// The pure tick model of the timeline scale, extracted for testability and
// as the future home of the physical-time axis mapping.

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
 * Smallest 1/2/5×10ⁿ step that is at least `minStep` frames.
 */
export function niceStep(minStep: number): number {
  let magnitude = 10 ** Math.floor(Math.log10(Math.max(minStep, 1)));
  for (;;) {
    for (const multiplier of [1, 2, 5]) {
      const step = multiplier * magnitude;
      if (step >= minStep) return step;
    }
    magnitude *= 10;
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/** Minimum pixel spacing between labeled ticks. */
const LABEL_SPACING = 48;

/** Minimum pixel spacing between minor ticks. */
const MINOR_SPACING = 6;

/**
 * Frame indices of the scale ticks: labeled major ticks and unlabeled
 * minor ticks, chosen so both respect their minimum pixel spacing.
 */
export interface FrameTicks {
  minorTicks: number[];
  labeledTicks: number[];
}

/**
 * Compute the scale ticks for the given frame count and density.
 */
export function computeFrameTicks(
  numFrames: number,
  pxPerFrame: number,
): FrameTicks {
  const labelStep = niceStep(LABEL_SPACING / pxPerFrame);
  const minorStep = niceStep(MINOR_SPACING / pxPerFrame);

  const minorTicks: number[] = [];
  const labeledTicks: number[] = [];
  for (let index = 0; index < numFrames; index += minorStep) {
    if (index % labelStep === 0) labeledTicks.push(index);
    else minorTicks.push(index);
  }
  return { minorTicks, labeledTicks };
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
