/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { Scene, WebGLRenderer } from "three";

import {
  type BackgroundColorName,
  backgroundColors,
} from "~/visual/background-color";
import { CameraController } from "~/visual/camera-controller";
import { Particles } from "~/visual/particles";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export class Renderer {
  readonly canvas: HTMLCanvasElement;
  readonly renderer: WebGLRenderer;
  readonly scene: Scene;
  readonly cameraController: CameraController;
  readonly particles: Particles;

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

    this.particles = new Particles();
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
    this.cameraController.camera.aspect = width / height;
    this.cameraController.camera.updateProjectionMatrix();
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  public setBackgroundColor(backgroundColorName: BackgroundColorName) {
    const backgroundColor = backgroundColors[backgroundColorName].color;
    if (backgroundColor === null) {
      this.renderer.setClearColor(0, 0);
    } else {
      this.renderer.setClearColor(backgroundColor, 1);
    }
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
