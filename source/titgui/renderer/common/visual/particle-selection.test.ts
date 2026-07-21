/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { Box2, Vector2 } from "three";
import { beforeEach, describe, expect, it } from "vitest";

import { Camera } from "~/renderer/common/visual/camera";
import { ParticleSelection } from "~/renderer/common/visual/particle-selection";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// A 100×100 viewport looking down the -Z axis at the [-1, 1]² square:
// world (x, y) maps to screen ((x + 1) · 50, (1 - y) · 50).
const VIEWPORT = 100;

function makeCamera() {
  const camera = new Camera();
  // With a 90° field of view at z = 1, the orthographic half-height is 1.
  camera.fov = 90;
  camera.position.set(0, 0, 1);
  camera.updateProjectionMatrix();
  camera.updateMatrixWorld();
  return camera;
}

// Particles on the diagonal: world (-0.5, -0.5), (0, 0), (0.5, 0.5) →
// screen (25, 75), (50, 50), (75, 25).
const positions = new Float32Array([-0.5, -0.5, 0, 0, 0, 0, 0.5, 0.5, 0]);

// Screen-space boxes around single particles.
const aroundFirst = new Box2(new Vector2(20, 70), new Vector2(30, 80));
const aroundSecond = new Box2(new Vector2(45, 45), new Vector2(55, 55));
const aroundAll = new Box2(new Vector2(0, 0), new Vector2(100, 100));

let selection: ParticleSelection;
const camera = makeCamera();

beforeEach(() => {
  selection = new ParticleSelection();
  selection.setViewportSize(VIEWPORT, VIEWPORT);
  selection.setData(positions);
});

const selectedIndices = () =>
  Array.from(selection.getValues())
    .map((value, index) => (value > 0 ? index : -1))
    .filter((index) => index >= 0);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

describe("ParticleSelection", () => {
  it("replaces the selection with particles inside the shape", () => {
    const count = selection.apply(camera, {
      action: "replace",
      shape: aroundFirst,
    });
    expect(count).toBe(1);
    expect(selectedIndices()).toEqual([0]);

    // A replace forgets the previous selection.
    expect(
      selection.apply(camera, { action: "replace", shape: aroundSecond }),
    ).toBe(1);
    expect(selectedIndices()).toEqual([1]);
  });

  it("adds and subtracts", () => {
    selection.apply(camera, { action: "replace", shape: aroundFirst });
    expect(
      selection.apply(camera, { action: "add", shape: aroundSecond }),
    ).toBe(2);
    expect(selectedIndices()).toEqual([0, 1]);

    expect(
      selection.apply(camera, { action: "subtract", shape: aroundFirst }),
    ).toBe(1);
    expect(selectedIndices()).toEqual([1]);
  });

  it("selects everything inside a covering shape and clears", () => {
    expect(
      selection.apply(camera, { action: "replace", shape: aroundAll }),
    ).toBe(3);
    expect(selection.apply(camera, { action: "clear" })).toBe(0);
    expect(selectedIndices()).toEqual([]);
  });

  it("does not double-count re-added particles", () => {
    selection.apply(camera, { action: "replace", shape: aroundAll });
    expect(
      selection.apply(camera, { action: "add", shape: aroundSecond }),
    ).toBe(3);
  });

  it("resets the selection when the particle count changes", () => {
    selection.apply(camera, { action: "replace", shape: aroundAll });
    selection.setData(new Float32Array(6));
    expect(selectedIndices()).toEqual([]);
  });
});

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
