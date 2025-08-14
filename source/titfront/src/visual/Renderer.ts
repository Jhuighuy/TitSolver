/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { Scene, WebGLRenderer } from "three";

import { assert } from "~/utils";
import {
  type BackgroundColorName,
  backgroundColors,
} from "~/visual/BackroundColor";
import { CameraController } from "~/visual/CameraController";
import type { ColorMapName } from "~/visual/ColorMap";
import type { Frame } from "~/visual/Frame";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export class Renderer {
  readonly canvas: HTMLCanvasElement;
  readonly renderer: WebGLRenderer;
  readonly scene: Scene;
  readonly cameraController: CameraController;

  public frames: Frame[] = [];
  public backgroundColor: BackgroundColorName = "none";
  public colorMap: ColorMapName = "jet";
  public particleSize: number = 10;

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  public constructor(canvas: HTMLCanvasElement) {
    // Setup canvas.
    assert(canvas.parentElement !== null);
    this.canvas = canvas;

    // Create WebGL renderer.
    this.renderer = new WebGLRenderer({ canvas, antialias: true });
    this.renderer.setClearAlpha(0);
    this.renderer.setAnimationLoop(() => this.animate());

    // Setup an empty scene.
    this.scene = new Scene();

    // Setup camera and controls.
    this.cameraController = new CameraController(this.canvas);
    this.scene.add(this.cameraController);
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  public addFrame(frame: Frame) {
    for (const frame of this.frames) frame.removeFrom(this.scene);
    this.frames = [];
    this.frames.push(frame);

    frame.addTo(this.scene);
    frame.setColorMap(this.colorMap);
  }

  public setColorMap(colorMap: ColorMapName) {
    this.colorMap = colorMap;
    for (const frame of this.frames) frame.setColorMap(colorMap);
  }

  public setColorRange(min: number, max: number) {
    for (const frame of this.frames) frame.setColorRange(min, max);
  }

  public setParticleSize(size: number) {
    this.particleSize = size;
    for (const frame of this.frames) frame.setParticleSize(this.particleSize);
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  public dispose() {
    this.cameraController.dispose();
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  private animate() {
    const backgroundColorValue = backgroundColors[this.backgroundColor].color;
    if (backgroundColorValue !== null) {
      this.renderer.setClearColor(backgroundColorValue, /*alpha=*/ 1);
    } else {
      this.renderer.setClearColor(0, /*alpha=*/ 0);
    }
    this.renderer.clearColor();

    this.renderer.render(this.scene, this.cameraController.camera);
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  public resize(width: number, height: number) {
    this.renderer.setSize(width, height, false);
    this.cameraController.camera.aspect = width / height;
    this.cameraController.camera.updateProjectionMatrix();
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
