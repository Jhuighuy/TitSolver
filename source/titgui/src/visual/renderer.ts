/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { Scene, Vector2, Vector3, WebGLRenderer } from "three";

import { assert } from "~/utils";
import type { BackgroundColor } from "~/visual/background-color";
import type { Projection } from "~/visual/camera";
import { CameraController } from "~/visual/camera-controller";
import {
  type ColorMap,
  type ColorRange,
  colorRangeDefault,
} from "~/visual/color-map";
import type { Field, FieldMap, FieldModifier } from "~/visual/fields";
import type { GlyphScaleMode } from "~/visual/glyphs";
import { ParticlesSwitch, type RenderMode } from "~/visual/particles-switch";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export class Renderer {
  private readonly canvas: HTMLCanvasElement;
  private readonly renderer: WebGLRenderer;
  private positionValues = new Float32Array();
  private selectionValues = new Float32Array();
  private renderWidth = 1;
  private renderHeight = 1;
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
    this.renderWidth = Math.max(width, 1);
    this.renderHeight = Math.max(height, 1);
    this.renderer.setSize(width, height, false);
    this.cameraController.camera.aspect = this.renderWidth / this.renderHeight;
    this.cameraController.camera.updateProjectionMatrix();
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
    this.positionValues = positionValues;
    if (this.selectionValues.length !== data.count) {
      this.selectionValues = new Float32Array(data.count);
    }
    this.particles.setSelection(this.selectionValues);

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

  public setSelectedParticleIndices(indices: readonly number[]) {
    const count = this.positionValues.length / 3;
    this.selectionValues = new Float32Array(count);
    for (const index of indices) {
      if (0 <= index && index < count) {
        this.selectionValues[index] = 1;
      }
    }
    this.particles.setSelection(this.selectionValues);
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  public selectParticlesInRect(
    start: Vector2,
    end: Vector2,
  ): number[] {
    const left = Math.min(start.x, end.x);
    const right = Math.max(start.x, end.x);
    const top = Math.min(start.y, end.y);
    const bottom = Math.max(start.y, end.y);

    return this.selectProjectedParticles(
      ({ x, y }) => left <= x && x <= right && top <= y && y <= bottom,
    );
  }

  public selectParticlesInPolygon(points: Vector2[]): number[] {
    if (points.length < 3) return [];
    return this.selectProjectedParticles((point) => pointInPolygon(point, points));
  }

  private selectProjectedParticles(
    predicate: (point: Vector2) => boolean,
  ): number[] {
    const camera = this.cameraController.camera;
    const worldPosition = new Vector3();
    const projectedPosition = new Vector3();
    const screenPoint = new Vector2();
    const selectedIndices: number[] = [];

    for (let index = 0; index < this.positionValues.length / 3; index++) {
      worldPosition.set(
        this.positionValues[index * 3 + 0] ?? 0,
        this.positionValues[index * 3 + 1] ?? 0,
        this.positionValues[index * 3 + 2] ?? 0,
      );

      projectedPosition.copy(worldPosition).project(camera);
      if (!Number.isFinite(projectedPosition.x)) continue;
      if (!Number.isFinite(projectedPosition.y)) continue;
      if (!Number.isFinite(projectedPosition.z)) continue;
      if (projectedPosition.z < -1 || projectedPosition.z > 1) continue;

      screenPoint.set(
        (projectedPosition.x * 0.5 + 0.5) * this.renderWidth,
        (1 - (projectedPosition.y * 0.5 + 0.5)) * this.renderHeight,
      );

      if (predicate(screenPoint)) selectedIndices.push(index);
    }

    return selectedIndices;
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

function pointInPolygon(point: Vector2, polygon: Vector2[]) {
  let inside = false;

  for (let i = 0, j = polygon.length - 1; i < polygon.length; j = i++) {
    const pi = polygon[i];
    const pj = polygon[j];
    if (pi === undefined || pj === undefined) continue;

    const intersects =
      (pi.y > point.y) !== (pj.y > point.y) &&
      point.x <
        ((pj.x - pi.x) * (point.y - pi.y)) / (pj.y - pi.y + Number.EPSILON) +
          pi.x;
    if (intersects) inside = !inside;
  }

  return inside;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
