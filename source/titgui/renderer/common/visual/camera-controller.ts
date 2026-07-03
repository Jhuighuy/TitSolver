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

import { clamp } from "~/renderer/common/utils-math";
import { Camera } from "~/renderer/common/visual/camera";

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
  public minZoom = 1e-3;
  public maxZoom = 1e6;

  private readonly controller = new AbortController();

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  public constructor(canvas: HTMLCanvasElement) {
    super();

    this.canvas = canvas;
    this.camera = new Camera();
    this.camera.position.set(0, 0, 5);
    this.add(this.camera);

    this.setupMouseEventListeners();
  }

  public dispose() {
    this.controller.abort();
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  public setViewportSize(width: number, height: number) {
    this.camera.aspect = width / height;
    this.camera.updateProjectionMatrix();
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
    this.camera.zoom = clamp(
      this.camera.zoom * (1 + dy * this.zoomSpeed),
      this.minZoom,
      this.maxZoom,
    );
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

  private setupMouseEventListeners() {
    this.setupClickEventListener();
    this.setupDoubleClickEventListener();
    this.setupWheelEventListener();
  }

  private setupClickEventListener() {
    this.canvas.addEventListener(
      "mousedown",
      (event) => {
        if (event.button > 2) return; // Ignore non-primary mouse buttons.
        event.preventDefault();

        const start = new Vector2(event.clientX, event.clientY);
        const end = new Vector2();

        const moveController = new AbortController();

        globalThis.addEventListener(
          "mousemove",
          (moveEvent) => {
            end.set(moveEvent.clientX, moveEvent.clientY);
            const dx = (end.x - start.x) / this.canvas.clientWidth;
            const dy = (end.y - start.y) / this.canvas.clientHeight;
            switch (event.button) {
              case 0:
                this.panCamera(dx, dy);
                break;
              case 1:
                this.zoomCamera(dy);
                break;
              case 2:
                this.rotateCamera(dx, dy);
                break;
            }
            start.copy(end);
            this.dispatchEvent({ type: "changed" });
          },
          {
            signal: AbortSignal.any([
              this.controller.signal,
              moveController.signal,
            ]),
          },
        );

        globalThis.addEventListener(
          "mouseup",
          () => {
            moveController.abort();
          },
          { signal: this.controller.signal },
        );
      },
      { signal: this.controller.signal },
    );
  }

  private setupDoubleClickEventListener() {
    this.canvas.addEventListener(
      "dblclick",
      () => {
        this.snapToNearestAxis();
        this.dispatchEvent({ type: "changed" });
      },
      { signal: this.controller.signal },
    );
  }

  private setupWheelEventListener() {
    this.canvas.addEventListener(
      "wheel",
      (event) => {
        event.preventDefault();

        // Normalize wheel delta to CSS pixels for consistent mouse/touchpad
        // behavior.
        const deltaPixels = (() => {
          switch (event.deltaMode) {
            case WheelEvent.DOM_DELTA_PIXEL:
              return event.deltaY;
            case WheelEvent.DOM_DELTA_LINE:
              return event.deltaY * 16;
            case WheelEvent.DOM_DELTA_PAGE:
              return event.deltaY * this.canvas.clientHeight;
            default:
              return event.deltaY;
          }
        })();

        // Exponential scale gives smooth trackpad zoom and stable wheel step
        // zoom.
        const sensitivity = 0.002 * this.zoomSpeed;
        const delta = Math.exp(-deltaPixels * sensitivity);
        this.camera.zoom = clamp(
          this.camera.zoom * delta,
          this.minZoom,
          this.maxZoom,
        );

        this.camera.updateProjectionMatrix();
        this.dispatchEvent({ type: "changed" });
      },
      { signal: this.controller.signal, passive: false },
    );
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
