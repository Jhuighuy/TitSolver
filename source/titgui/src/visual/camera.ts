/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { MathUtils, Camera as ThreeCamera } from "three";

import { assert } from "~/utils";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export type Projection = "orthographic" | "perspective";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export class Camera extends ThreeCamera {
  public projection: Projection = "orthographic";
  public aspect: number = 1;
  public near: number = 0.01;
  public far: number = 1000;
  public fov: number = 30;
  public zoom: number = 1;

  public updateProjectionMatrix() {
    const fovRad = MathUtils.degToRad(this.fov);
    const anyHeight = (2 * Math.tan(fovRad / 2)) / this.zoom;
    switch (this.projection) {
      case "orthographic": {
        // Note: this projection switch "hack" is intended to be used only
        //       for cameras "focusing" on the parent object.
        //                              vvvvvvvvvvvvvvv
        const halfHeight = (anyHeight * this.position.z) / 2;
        const halfWidth = halfHeight * this.aspect;
        this.projectionMatrix.makeOrthographic(
          -halfWidth,
          halfWidth,
          halfHeight,
          -halfHeight,
          this.near,
          this.far
        );
        break;
      }
      case "perspective": {
        const halfHeight = (anyHeight * this.near) / 2;
        const halfWidth = halfHeight * this.aspect;
        this.projectionMatrix.makePerspective(
          -halfWidth,
          halfWidth,
          halfHeight,
          -halfHeight,
          this.near,
          this.far
        );
        break;
      }
      default:
        assert(false);
    }
    this.projectionMatrixInverse.copy(this.projectionMatrix).invert();
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
