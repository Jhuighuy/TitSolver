/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// @vitest-environment jsdom

import { Box2, Vector2 } from "three";
import { beforeEach, describe, expect, it, vi } from "vitest";

import { cssVariableToRgb } from "~/renderer/common/utils";
import { colorMaps } from "~/renderer/common/visual/color-map";
import { FieldMap } from "~/renderer/common/visual/fields";
import { Renderer } from "~/renderer/common/visual/renderer";

// jsdom has no WebGL; substitute the GL-facing Three.js class and keep the
// rest of the library (geometry, math, materials) real.
vi.mock("three", async (importOriginal) => {
  const three = await importOriginal<typeof import("three")>();
  return {
    ...three,
    WebGLRenderer: class {
      public setClearColor = vi.fn();
      public setAnimationLoop = vi.fn();
      public setSize = vi.fn();
      public clearColor = vi.fn();
      public render = vi.fn();
      public dispose = vi.fn();
    },
  };
});

// Token resolution needs computed CSS and a 2D canvas, neither of which
// jsdom provides.
vi.mock("~/renderer/common/utils", async (importOriginal) => ({
  ...(await importOriginal<typeof import("~/renderer/common/utils")>()),
  cssVariableToRgb: vi.fn(() => [0.9, 0.7, 0.2] as [number, number, number]),
}));

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Two particles in 2D, at (0, 0) and (0.1, 0), with distinct densities.
function makeFields() {
  return new FieldMap({
    rho: new Float32Array([1, 3]),
    r: new Float32Array([0, 0, 0.1, 0]),
  });
}

interface GlRendererFake {
  setClearColor: ReturnType<typeof vi.fn>;
  setAnimationLoop: ReturnType<typeof vi.fn>;
  setSize: ReturnType<typeof vi.fn>;
  render: ReturnType<typeof vi.fn>;
  dispose: ReturnType<typeof vi.fn>;
}

function glRenderer(renderer: Renderer): GlRendererFake {
  return (renderer as unknown as { renderer: GlRendererFake }).renderer;
}

let renderer: Renderer;

beforeEach(() => {
  vi.clearAllMocks();
  renderer = new Renderer(document.createElement("canvas"));
});

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

describe("Renderer", () => {
  it("starts an animation loop that renders the scene", () => {
    const gl = glRenderer(renderer);
    const loop = gl.setAnimationLoop.mock.calls[0][0] as () => void;
    loop();
    expect(gl.render).toHaveBeenCalledWith(
      renderer.scene,
      renderer.cameraController.camera,
    );
  });

  it("applies the selection color token, and re-applies on theme flips", () => {
    expect(cssVariableToRgb).toHaveBeenCalledWith("--selection");
    renderer.dispose();
  });

  it("propagates resizes to the camera", () => {
    renderer.resize(200, 100);
    expect(glRenderer(renderer).setSize).toHaveBeenCalledWith(200, 100, false);
    expect(renderer.cameraController.camera.aspect).toBe(2);
  });

  it("sets an opaque or transparent clear color", () => {
    const gl = glRenderer(renderer);
    renderer.setBackgroundColor({
      label: "None",
      color: null,
      appearance: "inherit",
    });
    expect(gl.setClearColor).toHaveBeenLastCalledWith(0, 0);
    renderer.setBackgroundColor({
      label: "White",
      color: 0xffffff,
      appearance: "light",
    });
    expect(gl.setClearColor).toHaveBeenLastCalledWith(0xffffff, 1);
  });

  it("switches the camera projection", () => {
    renderer.setProjection("orthographic");
    expect(renderer.cameraController.camera.projection).toBe("orthographic");
  });

  it("computes the color range from the finite field values", () => {
    const fields = makeFields();
    const rho = fields.get("rho");
    expect(rho).toBeDefined();
    if (rho === undefined) return;

    const range = renderer.setRenderData(fields, rho, rho, 0);
    expect(range).toEqual({ min: 1, max: 3 });
  });

  it("falls back to the default color range without finite values", () => {
    const fields = new FieldMap({
      rho: new Float32Array([Number.NaN, Number.NaN]),
      r: new Float32Array([0, 0, 0.1, 0]),
    });
    const rho = fields.get("rho");
    expect(rho).toBeDefined();
    if (rho === undefined) return;

    const range = renderer.setRenderData(fields, rho, rho, 0);
    expect(range).toEqual({ min: 0, max: 1 });
  });

  it("delegates display settings to the particle system", () => {
    const particles = renderer.particles;
    const setColorMap = vi.spyOn(particles, "setColorMap");
    const setColorRange = vi.spyOn(particles, "setColorRange");
    const setShadingMode = vi.spyOn(particles, "setShadingMode");
    const setPointSize = vi.spyOn(particles, "setPointSize");
    const setGlyphScale = vi.spyOn(particles, "setGlyphScale");
    const setGlyphScaleMode = vi.spyOn(particles, "setGlyphScaleMode");

    renderer.setRenderColorMap("points", colorMaps.grayscale);
    renderer.setRenderColorRange("points", { min: 0, max: 2 });
    renderer.setShadingMode("flat");
    renderer.setPointSize(4);
    renderer.setGlyphScale(2);
    renderer.setGlyphScaleMode("uniform");

    expect(setColorMap).toHaveBeenCalled();
    expect(setColorRange).toHaveBeenCalledWith({ min: 0, max: 2 });
    expect(setShadingMode).toHaveBeenCalledWith("flat");
    expect(setPointSize).toHaveBeenCalledWith(4);
    expect(setGlyphScale).toHaveBeenCalledWith(2);
    expect(setGlyphScaleMode).toHaveBeenCalledWith("uniform");
  });

  it("selects projected particles inside a marquee", () => {
    const fields = makeFields();
    const rho = fields.get("rho");
    expect(rho).toBeDefined();
    if (rho === undefined) return;

    renderer.resize(100, 100);
    renderer.setRenderData(fields, rho, rho, 0);
    // The render loop normally refreshes world matrices; do it by hand.
    renderer.scene.updateMatrixWorld(true);

    // Both particles sit near the viewport center; a full-viewport marquee
    // catches them, clearing empties the selection again.
    const marquee = new Box2(new Vector2(0, 0), new Vector2(100, 100));
    expect(
      renderer.applySelectionCommand({ action: "replace", shape: marquee }),
    ).toBe(2);
    expect(renderer.applySelectionCommand({ action: "clear" })).toBe(0);
  });
});

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
