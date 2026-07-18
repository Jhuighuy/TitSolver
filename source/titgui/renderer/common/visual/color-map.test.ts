/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { describe, expect, it } from "vitest";

import {
  colorMaps,
  colorMapToTexture,
} from "~/renderer/common/visual/color-map";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

describe("colorMapToTexture", () => {
  it("samples the control points at the ends", () => {
    const texture = colorMapToTexture(colorMaps.grayscale, 4);
    const data = texture.image.data as Uint8Array;

    // Grayscale runs black to white with full alpha.
    expect(Array.from(data.slice(0, 4))).toEqual([0, 0, 0, 255]);
    expect(Array.from(data.slice(-4))).toEqual([255, 255, 255, 255]);
  });

  it("interpolates between control points", () => {
    const texture = colorMapToTexture(colorMaps.grayscale, 3);
    const data = texture.image.data as Uint8Array;
    expect(Array.from(data.slice(4, 8))).toEqual([128, 128, 128, 255]);
  });

  it("builds a well-formed texture for every predefined map", () => {
    for (const colorMap of Object.values(colorMaps)) {
      const texture = colorMapToTexture(colorMap);
      expect(texture.image.width).toBe(256);
      expect(texture.image.height).toBe(1);
      expect(texture.image.data).toHaveLength(256 * 4);
      // Alpha stays opaque throughout.
      const data = texture.image.data as Uint8Array;
      for (let i = 3; i < data.length; i += 4) expect(data[i]).toBe(255);
    }
  });
});

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
