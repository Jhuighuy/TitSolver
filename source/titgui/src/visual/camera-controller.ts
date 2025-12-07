/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import {
  Object3D,
  type Object3DEventMap,
  Quaternion,
  Vector2,
  Vector3,
} from "three";

import { assert } from "~/utils";
import { Camera } from "~/visual/camera";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export interface CameraControllerEventMap extends Object3DEventMap {
  changed: { type: "changed" };
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export class CameraController extends Object3D<CameraControllerEventMap> {
  public readonly canvas: HTMLCanvasElement;
  public readonly camera: Camera;

  public rotateSpeed = 2;
  public zoomSpeed = 1.2;
  public panSpeed = 2;

  private state: "pan" | "zoom" | "rotate" | null = null;
  private readonly eventStart = new Vector2();
  private readonly eventEnd = new Vector2();

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  public constructor(canvas: HTMLCanvasElement) {
    super();

    this.canvas = canvas;
    this.camera = new Camera();
    this.camera.position.set(0, 0, 5);
    this.add(this.camera);

    canvas.addEventListener("mousedown", this.onMouseDown);
    canvas.addEventListener("dblclick", this.onDoubleClick);
    canvas.addEventListener("wheel", this.onMouseWheel, { passive: false });
  }

  public dispose() {
    this.state = null;
    this.canvas.removeEventListener("mousedown", this.onMouseDown);
    this.canvas.removeEventListener("dblclick", this.onDoubleClick);
    this.canvas.removeEventListener("wheel", this.onMouseWheel);
    globalThis.removeEventListener("mousemove", this.onMouseMove);
    globalThis.removeEventListener("mouseup", this.onMouseUp);
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  private panCamera(dx: number, dy: number) {
    const panX = -dx * (this.panSpeed / this.camera.zoom);
    const panY = dy * (this.panSpeed / this.camera.zoom);
    const right = new Vector3(1, 0, 0).applyQuaternion(this.quaternion);
    const up = new Vector3(0, 1, 0).applyQuaternion(this.quaternion);
    this.position.addScaledVector(right, panX);
    this.position.addScaledVector(up, panY);
  }

  private zoomCamera(dy: number) {
    this.camera.zoom *= 1 + dy * this.zoomSpeed;
    this.camera.updateProjectionMatrix();
  }

  private rotateCamera(dx: number, dy: number) {
    const up = new Vector3(0, 1, 0);
    const yaw = -dx * this.rotateSpeed;
    const yawQuat = new Quaternion().setFromAxisAngle(up, yaw);
    const right = new Vector3(1, 0, 0).applyQuaternion(this.quaternion);
    const pitch = -dy * this.rotateSpeed;
    const pitchQuat = new Quaternion().setFromAxisAngle(right, pitch);
    this.quaternion.copy(yawQuat.multiply(pitchQuat).multiply(this.quaternion));
  }

  private snapToNearestAxis() {
    const forward = new Vector3(0, 0, -1).applyQuaternion(this.quaternion);
    const axes = [
      new Vector3(1, 0, 0),
      new Vector3(-1, 0, 0),
      new Vector3(0, 1, 0),
      new Vector3(0, -1, 0),
      new Vector3(0, 0, 1),
      new Vector3(0, 0, -1),
    ];
    let best = axes[0];
    let maxDot = Number.NEGATIVE_INFINITY;
    for (const axis of axes) {
      const dot = forward.dot(axis);
      if (dot > maxDot) {
        maxDot = dot;
        best = axis;
      }
    }
    this.lookAt(this.position.clone().sub(best));
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  private readonly onMouseDown = (event: MouseEvent) => {
    event.preventDefault();
    if (this.state !== null) return;

    this.eventStart.set(event.clientX, event.clientY);
    switch (event.button) {
      case 0:
        this.state = "pan";
        break;
      case 1:
        this.state = "zoom";
        break;
      case 2:
        this.state = "rotate";
        break;
      default:
        return; // Ignore other buttons.
    }

    globalThis.addEventListener("mousemove", this.onMouseMove);
    globalThis.addEventListener("mouseup", this.onMouseUp);
  };

  private readonly onMouseMove = (event: MouseEvent) => {
    this.eventEnd.set(event.clientX, event.clientY);
    const dx = (this.eventEnd.x - this.eventStart.x) / this.canvas.clientWidth;
    const dy = (this.eventEnd.y - this.eventStart.y) / this.canvas.clientHeight;
    assert(this.state !== null);
    switch (this.state) {
      case "pan":
        this.panCamera(dx, dy);
        break;
      case "zoom":
        this.zoomCamera(dy);
        break;
      case "rotate":
        this.rotateCamera(dx, dy);
        break;
      default:
        assert(false);
    }
    this.eventStart.copy(this.eventEnd);
    this.dispatchEvent({ type: "changed" });
  };

  private readonly onMouseUp = () => {
    this.state = null;
    globalThis.removeEventListener("mousemove", this.onMouseMove);
    globalThis.removeEventListener("mouseup", this.onMouseUp);
  };

  private readonly onDoubleClick = () => {
    this.snapToNearestAxis();
    this.dispatchEvent({ type: "changed" });
  };

  private readonly onMouseWheel = (event: WheelEvent) => {
    event.preventDefault();
    const delta = event.deltaY > 0 ? 1 / this.zoomSpeed : this.zoomSpeed;
    this.camera.zoom *= delta;
    this.camera.updateProjectionMatrix();
    this.dispatchEvent({ type: "changed" });
  };
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
