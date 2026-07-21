/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { createStore } from "jotai";
import { describe, expect, it } from "vitest";

import { FieldMap } from "~/renderer/common/visual/fields";
import {
  createViewportState,
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
    const viewport = createViewportState();
    store.set(frameDataAtom, makeFrameData());
    store.set(viewport.fieldNameAtom, "v");

    expect(store.get(viewport.fieldAtom).name).toBe("v");
  });

  it("falls back to density when the selection is missing", () => {
    const store = createStore();
    const viewport = createViewportState();
    store.set(frameDataAtom, makeFrameData());
    store.set(viewport.fieldNameAtom, "v");
    store.set(frameDataAtom, new FieldMap({}));

    expect(store.get(viewport.fieldAtom).name).toBe("rho");
    expect(store.get(viewport.colorFieldAtom).name).toBe("rho");
  });

  it("returns to the selection once the field is available again", () => {
    const store = createStore();
    const viewport = createViewportState();
    store.set(viewport.fieldNameAtom, "v");
    store.set(frameDataAtom, new FieldMap({}));
    expect(store.get(viewport.fieldAtom).name).toBe("rho");

    store.set(frameDataAtom, makeFrameData());
    expect(store.get(viewport.fieldAtom).name).toBe("v");
  });

  it("keeps independent selections across viewport instances", () => {
    const store = createStore();
    const first = createViewportState();
    const second = createViewportState();
    store.set(frameDataAtom, makeFrameData());
    store.set(first.fieldNameAtom, "v");

    expect(store.get(first.fieldAtom).name).toBe("v");
    expect(store.get(second.fieldAtom).name).toBe("rho");
  });
});

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
