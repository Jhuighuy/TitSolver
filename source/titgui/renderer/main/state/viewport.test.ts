/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { createStore } from "jotai";
import { describe, expect, it } from "vitest";

import { FieldMap } from "~/renderer/common/visual/fields";
import {
  colorFieldAtom,
  fieldAtom,
  fieldNameAtom,
  frameDataAtom,
} from "~/renderer/main/state/viewport";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Two particles in 2D with a scalar density and a vector velocity.
function makeFrameData() {
  return new FieldMap({
    rho: new Float32Array(2),
    r: new Float32Array(4),
    v: new Float32Array(4),
  });
}

describe("field selection", () => {
  it("resolves the selected field when present", () => {
    const store = createStore();
    store.set(frameDataAtom, makeFrameData());
    store.set(fieldNameAtom, "v");

    expect(store.get(fieldAtom).name).toBe("v");
  });

  it("falls back to density when the selection is missing", () => {
    const store = createStore();
    store.set(frameDataAtom, makeFrameData());
    store.set(fieldNameAtom, "v");
    store.set(frameDataAtom, new FieldMap({}));

    expect(store.get(fieldAtom).name).toBe("rho");
    expect(store.get(colorFieldAtom).name).toBe("rho");
  });

  it("returns to the selection once the field is available again", () => {
    const store = createStore();
    store.set(fieldNameAtom, "v");
    store.set(frameDataAtom, new FieldMap({}));
    expect(store.get(fieldAtom).name).toBe("rho");

    store.set(frameDataAtom, makeFrameData());
    expect(store.get(fieldAtom).name).toBe("v");
  });
});

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
