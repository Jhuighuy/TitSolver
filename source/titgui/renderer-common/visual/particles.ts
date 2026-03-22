/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import type { Object3D } from "three";

import type { ColorMap, ColorRange } from "~/renderer-common/visual/color-map";
import type { Field } from "~/renderer-common/visual/fields";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export type ShadingMode = "flat" | "shaded";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export interface Particles extends Object3D {
  dispose(): void;
  setData(
    field: Field,
    colorValues: Float32Array,
    positionValues: Float32Array,
  ): void;
  setShadingMode(mode: ShadingMode): void;
  setColorRange(colorRange: ColorRange): void;
  setColorMap(colorMap: ColorMap): void;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
