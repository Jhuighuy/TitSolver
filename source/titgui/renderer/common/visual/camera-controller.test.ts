/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// @vitest-environment jsdom

import { Vector3 } from "three";
import { afterEach, beforeEach, describe, expect, it, vi } from "vitest";

import { CameraController } from "~/renderer/common/visual/camera-controller";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

let canvas: HTMLCanvasElement;
let controller: CameraController;
let changed: ReturnType<typeof vi.fn<() => void>>;

function mouse(type: string, options: MouseEventInit = {}) {
  return new MouseEvent(type, { bubbles: true, ...options });
}

function drag(button: number, dx: number, dy: number) {
  canvas.dispatchEvent(mouse("mousedown", { button, clientX: 0, clientY: 0 }));
  window.dispatchEvent(mouse("mousemove", { clientX: dx, clientY: dy }));
  window.dispatchEvent(mouse("mouseup"));
}

function cameraForward() {
  return new Vector3(0, 0, -1).applyQuaternion(controller.quaternion);
}

beforeEach(() => {
  canvas = document.createElement("canvas");
  // jsdom has no layout; give the canvas a fixed CSS size.
  Object.defineProperty(canvas, "clientWidth", { value: 100 });
  Object.defineProperty(canvas, "clientHeight", { value: 100 });

  controller = new CameraController(canvas);
  changed = vi.fn<() => void>();
  controller.addEventListener("changed", () => {
    changed();
  });
});

afterEach(() => {
  controller.dispose();
});

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

describe("CameraController", () => {
  it("updates the camera aspect on viewport resize", () => {
    controller.setViewportSize(200, 100);
    expect(controller.camera.aspect).toBe(2);
  });

  it("pans with the primary button", () => {
    drag(0, 10, 0);
    // A rightward drag moves the pivot left (the scene follows the mouse).
    expect(controller.position.x).toBeCloseTo(-0.2);
    expect(controller.position.y).toBeCloseTo(0);
    expect(changed).toHaveBeenCalled();
  });

  it("zooms with the middle button", () => {
    drag(1, 0, 10);
    expect(controller.camera.zoom).toBeCloseTo(1.12);
  });

  it("rotates with the secondary button", () => {
    drag(2, 10, 0);
    const forward = cameraForward();
    expect(forward.z).toBeLessThan(-0.9);
    expect(forward.x).not.toBeCloseTo(0, 5);
  });

  it("stops dragging on mouse up", () => {
    drag(0, 10, 0);
    const panned = controller.position.x;

    // A move after the mouse-up must be ignored.
    window.dispatchEvent(mouse("mousemove", { clientX: 50, clientY: 50 }));
    expect(controller.position.x).toBe(panned);
  });

  it("ignores non-primary mouse buttons", () => {
    drag(3, 10, 0);
    expect(controller.position.x).toBe(0);
    expect(changed).not.toHaveBeenCalled();
  });

  it("zooms with the wheel, clamped to the limits", () => {
    canvas.dispatchEvent(new WheelEvent("wheel", { deltaY: -100 }));
    expect(controller.camera.zoom).toBeGreaterThan(1);
    expect(changed).toHaveBeenCalled();

    // A line-mode wheel step scales like 16 pixels.
    const zoomBefore = controller.camera.zoom;
    canvas.dispatchEvent(new WheelEvent("wheel", { deltaY: -1, deltaMode: 1 }));
    expect(controller.camera.zoom).toBeGreaterThan(zoomBefore);

    // Extreme deltas hit the configured bounds.
    canvas.dispatchEvent(new WheelEvent("wheel", { deltaY: -1e7 }));
    expect(controller.camera.zoom).toBe(controller.maxZoom);
    canvas.dispatchEvent(new WheelEvent("wheel", { deltaY: 1e8 }));
    expect(controller.camera.zoom).toBe(controller.minZoom);
  });

  it("snaps to the nearest axis on double click", () => {
    // Nudge the view off-axis, then snap back.
    drag(2, 3, 2);
    canvas.dispatchEvent(mouse("dblclick"));

    const forward = cameraForward();
    const alignment = Math.max(
      Math.abs(forward.x),
      Math.abs(forward.y),
      Math.abs(forward.z),
    );
    expect(alignment).toBeCloseTo(1);
  });

  it("detaches all listeners on dispose", () => {
    controller.dispose();
    drag(0, 10, 0);
    expect(controller.position.x).toBe(0);
    expect(changed).not.toHaveBeenCalled();
  });
});

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
