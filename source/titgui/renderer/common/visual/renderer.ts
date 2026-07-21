/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { Scene, WebGLRenderer } from "three";

import { cssVariableToRgb } from "~/renderer/common/utils";
import type { BackgroundColor } from "~/renderer/common/visual/background-color";
import type { Projection } from "~/renderer/common/visual/camera";
import { CameraController } from "~/renderer/common/visual/camera-controller";
import {
  type ColorMap,
  type ColorRange,
  colorRangeDefault,
} from "~/renderer/common/visual/color-map";
import type {
  Field,
  FieldMap,
  FieldModifier,
} from "~/renderer/common/visual/fields";
import type { GlyphScaleMode } from "~/renderer/common/visual/glyphs";
import { ParticleSelection } from "~/renderer/common/visual/particle-selection";
import type { ShadingMode } from "~/renderer/common/visual/particles";
import {
  ParticlesSwitch,
  type RenderMode,
} from "~/renderer/common/visual/particles-switch";
import type { SelectionCommand } from "~/renderer/common/visual/selection";
import { assert } from "~/shared/utils";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
 * Check whether a WebGL 2 context can be created. Cheap probe used to skip
 * renderer construction on machines without GPU support (e.g. headless CI),
 * where Three.js would log errors and throw.
 */
export function isWebGLAvailable() {
  try {
    const canvas = document.createElement("canvas");
    return canvas.getContext("webgl2") !== null;
  } catch {
    return false;
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export class Renderer {
  private readonly canvas: HTMLCanvasElement;
  private readonly renderer: WebGLRenderer;
  private readonly selection = new ParticleSelection();
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

    // Selection highlight color comes from the design tokens. Re-read it when
    // the color scheme flips, in case the token ever differs per theme.
    this.particles.setSelectionColor(cssVariableToRgb("--selection"));
    this.colorSchemeQuery.addEventListener("change", this.onColorSchemeChange);
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  public dispose() {
    this.colorSchemeQuery.removeEventListener(
      "change",
      this.onColorSchemeChange,
    );
    this.particles.dispose();
    this.cameraController.dispose();
    this.renderer.setAnimationLoop(null);
    this.renderer.dispose();
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  private readonly colorSchemeQuery = globalThis.matchMedia(
    "(prefers-color-scheme: dark)",
  );

  private readonly onColorSchemeChange = () => {
    this.particles.setSelectionColor(cssVariableToRgb("--selection"));
  };

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  public resize(width: number, height: number) {
    this.renderer.setSize(width, height, false);
    this.cameraController.setViewportSize(width, height);
    this.selection.setViewportSize(width, height);
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
    assert(positionField !== undefined);
    const positionValues = new Float32Array(data.count * 3);
    for (let i = 0; i < data.count; i++) {
      const components = positionField.components(i);
      positionValues[i * 3 + 0] = components[0];
      positionValues[i * 3 + 1] = components[1];
      positionValues[i * 3 + 2] = components.at(2) ?? 0;
    }

    this.particles.setData(field, colorValues, positionValues);
    this.selection.setData(positionValues);
    this.particles.setSelection(this.selection.getValues());

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

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  public applySelectionCommand(command: SelectionCommand) {
    const selectionCount = this.selection.apply(
      this.cameraController.camera,
      command,
    );
    this.particles.setSelection(this.selection.getValues());
    return selectionCount;
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
