/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { Object3D } from "three";

import type { ColorMap, ColorRange } from "~/renderer-common/visual/color-map";
import type { Field } from "~/renderer-common/visual/fields";
import { type GlyphScaleMode, Glyphs } from "~/renderer-common/visual/glyphs";
import type {
  Particles,
  ShadingMode,
} from "~/renderer-common/visual/particles";
import { Spheres } from "~/renderer-common/visual/spheres";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export type RenderMode = "points" | "glyphs";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export class ParticlesSwitch extends Object3D implements Particles {
  private readonly spheres: Spheres;
  private readonly glyphs: Glyphs;
  private current: Particles;
  private mode: RenderMode;

  public constructor() {
    super();
    this.spheres = new Spheres();
    this.glyphs = new Glyphs();
    this.current = this.spheres;
    this.mode = "points";
    this.add(this.current);
  }

  public dispose() {
    this.spheres.dispose();
    this.glyphs.dispose();
  }

  public setRenderMode(mode: RenderMode) {
    if (mode === this.mode) return;
    this.mode = mode;

    this.remove(this.current);
    this.current = mode === "points" ? this.spheres : this.glyphs;
    this.add(this.current);
  }

  public setData(
    field: Field,
    colorValues: Float32Array,
    positionValues: Float32Array,
  ) {
    this.current.setData(field, colorValues, positionValues);
  }

  public setColorRange(colorRange: ColorRange) {
    this.current.setColorRange(colorRange);
  }

  public setColorMap(colorMap: ColorMap) {
    this.current.setColorMap(colorMap);
  }

  public setShadingMode(mode: ShadingMode) {
    this.current.setShadingMode(mode);
  }

  public setViewportSize(width: number, height: number) {
    this.spheres.setViewportSize(width, height);
  }

  public setPointSize(size: number) {
    this.spheres.setPointSize(size);
  }

  public setGlyphScale(scale: number) {
    this.glyphs.setScale(scale);
  }

  public setGlyphScaleMode(mode: GlyphScaleMode) {
    this.glyphs.setScaleMode(mode);
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export function isValidRenderMode(field: Field, mode: RenderMode) {
  return mode === "points" || field.rank === 1;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
