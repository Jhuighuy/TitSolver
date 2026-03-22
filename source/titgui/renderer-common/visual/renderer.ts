/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { Scene, WebGLRenderer } from "three";

import type { BackgroundColor } from "~/renderer-common/visual/background-color";
import type { Projection } from "~/renderer-common/visual/camera";
import { CameraController } from "~/renderer-common/visual/camera-controller";
import {
  type ColorMap,
  type ColorRange,
  colorRangeDefault,
} from "~/renderer-common/visual/color-map";
import type {
  Field,
  FieldMap,
  FieldModifier,
} from "~/renderer-common/visual/fields";
import type { GlyphScaleMode } from "~/renderer-common/visual/glyphs";
import type { ShadingMode } from "~/renderer-common/visual/particles";
import {
  ParticlesSwitch,
  type RenderMode,
} from "~/renderer-common/visual/particles-switch";
import { assert } from "~/shared/utils";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export class Renderer {
  private readonly canvas: HTMLCanvasElement;
  private readonly renderer: WebGLRenderer;
  readonly scene: Scene;
  readonly cameraController: CameraController;
  readonly particles: ParticlesSwitch;

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  public constructor(canvas: HTMLCanvasElement) {
    this.canvas = canvas;

    this.renderer = new WebGLRenderer({ canvas, antialias: true });
    this.renderer.setClearColor(0, 0);
    this.renderer.setAnimationLoop(() => {
      this.renderer.clearColor();
      this.renderer.render(this.scene, this.cameraController.camera);
    });

    this.scene = new Scene();

    this.cameraController = new CameraController(this.canvas);
    this.scene.add(this.cameraController);

    this.particles = new ParticlesSwitch();
    this.scene.add(this.particles);
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  public dispose() {
    this.particles.dispose();
    this.cameraController.dispose();
    this.renderer.setAnimationLoop(null);
    this.renderer.dispose();
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  public resize(width: number, height: number) {
    this.renderer.setSize(width, height, false);
    this.cameraController.setViewportSize(width, height);
    this.particles.setViewportSize(width, height);
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  public setBackgroundColor(backgroundColor: BackgroundColor) {
    const color = backgroundColor.color;
    if (color === null) this.renderer.setClearColor(0, 0);
    else this.renderer.setClearColor(color, 1);
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  public setProjection(projection: Projection) {
    this.cameraController.camera.projection = projection;
    this.cameraController.camera.updateProjectionMatrix();
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  public setRenderMode(mode: RenderMode) {
    this.particles.setRenderMode(mode);
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  public setRenderData(
    data: FieldMap,
    field: Field,
    colorField: Field,
    colorFieldModifier: FieldModifier,
  ): ColorRange {
    const colorValues = new Float32Array(data.count);
    const colorRange = {
      min: Number.POSITIVE_INFINITY,
      max: Number.NEGATIVE_INFINITY,
    };
    let hasFiniteColorValue = false;
    for (let i = 0; i < data.count; i++) {
      const value = colorField.value(i, colorFieldModifier);
      if (Number.isFinite(value)) {
        hasFiniteColorValue = true;
        colorValues[i] = value;
        colorRange.min = Math.min(colorRange.min, value);
        colorRange.max = Math.max(colorRange.max, value);
      } else {
        colorValues[i] = Number.NaN;
      }
    }

    const positionField = data.get("r");
    assert(positionField);
    const positionValues = new Float32Array(data.count * 3);
    for (let i = 0; i < data.count; i++) {
      const components = positionField.components(i);
      positionValues[i * 3 + 0] = components[0];
      positionValues[i * 3 + 1] = components[1];
      positionValues[i * 3 + 2] = components[2] ?? 0;
    }

    this.particles.setData(field, colorValues, positionValues);

    return hasFiniteColorValue ? colorRange : colorRangeDefault;
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  public setRenderColorMap(mode: RenderMode, colorMap: ColorMap) {
    this.setRenderMode(mode);
    this.particles.setColorMap(colorMap);
  }

  public setRenderColorRange(mode: RenderMode, colorRange: ColorRange) {
    this.setRenderMode(mode);
    this.particles.setColorRange(colorRange);
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  public setShadingMode(mode: ShadingMode) {
    this.particles.setShadingMode(mode);
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  public setPointSize(size: number) {
    this.particles.setPointSize(size);
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  public setGlyphScale(scale: number) {
    this.particles.setGlyphScale(scale);
  }

  public setGlyphScaleMode(mode: GlyphScaleMode) {
    this.particles.setGlyphScaleMode(mode);
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
