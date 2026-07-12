/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// The value shapes (kinds, types, arrays) live in the shared zod schemas —
// a single source of truth for the C++ ↔ TS contract. This file declares
// only the native object surfaces.
import type { NumericArray, Type } from "~/shared/storage";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export interface Storage {
  dataVersion(): Promise<number>;
  seriesCount(): Promise<number>;
  series(index: number): Promise<Series>;
  lastSeries(): Promise<Series>;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export interface Series {
  frameCount(): Promise<number>;
  frame(index: number): Promise<Frame>;
  readFrame(index: number): Promise<FrameData>;
  frameTimes(): Promise<Float64Array>;
  export(dirPath: string): Promise<void>;
}

/** All arrays of a frame, read in a single native call. */
export type FrameData = Record<string, { type: Type; data: NumericArray }>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export interface Frame {
  fields(): Promise<string[]>;
  field(name: string): Promise<Field>;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export interface Field {
  name(): Promise<string>;
  type(): Promise<Type>;
  data(): Promise<NumericArray>;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
