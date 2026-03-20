/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { Vector2, Vector3 } from "three";

import { Camera } from "~/renderer-common/visual/camera";
import type {
  SelectionAction,
  SelectionCommand,
} from "~/renderer-common/visual/selection";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

const selectionMixValue = 0.5;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export class ParticleSelection {
  private viewportWidth = 1;
  private viewportHeight = 1;
  private positionValues: Float32Array = new Float32Array();
  private selectionValues: Float32Array = new Float32Array();
  private selectionCount = 0;

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  public setViewportSize(width: number, height: number) {
    this.viewportWidth = Math.max(width, 1);
    this.viewportHeight = Math.max(height, 1);
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  public setData(positionValues: Float32Array) {
    this.positionValues = positionValues;
    if (this.selectionValues.length === positionValues.length / 3) return;

    this.selectionValues = new Float32Array(positionValues.length / 3);
    this.selectionCount = 0;
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  public getValues() {
    return this.selectionValues;
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  public apply(camera: Camera, command: SelectionCommand) {
    switch (command.action) {
      case "clear":
        return this.clear();
      case "replace":
      case "add":
      case "subtract":
        return this.updateProjected(
          camera,
          (point) => command.shape.containsPoint(point),
          command.action,
        );
    }
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  private clear() {
    if (this.selectionCount === 0) return 0;

    this.selectionValues.fill(0);
    this.selectionCount = 0;

    return this.selectionCount;
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  private updateProjected(
    camera: Camera,
    predicate: (point: Vector2) => boolean,
    action: SelectionAction,
  ) {
    if (action === "replace") {
      this.selectionValues.fill(0);
      this.selectionCount = 0;
    }

    const worldPosition = new Vector3();
    const projectedPosition = new Vector3();
    const screenPoint = new Vector2();
    for (let index = 0; index < this.positionValues.length / 3; index++) {
      worldPosition.set(
        this.positionValues[index * 3 + 0] ?? 0,
        this.positionValues[index * 3 + 1] ?? 0,
        this.positionValues[index * 3 + 2] ?? 0,
      );

      projectedPosition.copy(worldPosition).project(camera);
      if (projectedPosition.z < -1 || projectedPosition.z > 1) continue;

      screenPoint.set(
        (projectedPosition.x * 0.5 + 0.5) * this.viewportWidth,
        (1 - (projectedPosition.y * 0.5 + 0.5)) * this.viewportHeight,
      );
      if (!predicate(screenPoint)) continue;

      switch (action) {
        case "replace":
        case "add":
          if (this.selectionValues[index] === 0) {
            this.selectionValues[index] = selectionMixValue;
            this.selectionCount++;
          }
          break;
        case "subtract":
          if (this.selectionValues[index] !== 0) {
            this.selectionValues[index] = 0;
            this.selectionCount--;
          }
          break;
      }
    }

    return this.selectionCount;
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
