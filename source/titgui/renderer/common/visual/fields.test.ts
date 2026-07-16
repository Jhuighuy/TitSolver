/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { describe, expect, it } from "vitest";

import {
  FieldMap,
  fieldModifierDefault,
  fieldModifierToString,
  isValidFieldModifier,
} from "~/renderer/common/visual/fields";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Two particles in 2D: scalar rho, vector r/v, tensor S.
function makeFieldMap() {
  return new FieldMap({
    rho: new Float32Array(2),
    r: new Float32Array(4),
    v: new Float32Array(4),
    S: new Float32Array(8),
  });
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

describe("FieldMap", () => {
  it("infers count, dimension, and ranks", () => {
    const fields = makeFieldMap();
    expect(fields.count).toBe(2);
    expect(fields.dim).toBe(2);
    expect(fields.get("rho").rank).toBe(0);
    expect(fields.get("v").rank).toBe(1);
    expect(fields.get("S").rank).toBe(2);
  });

  it("synthesizes the virtual particle-id field", () => {
    const fields = makeFieldMap();
    expect(Array.from(fields.get("id").data)).toEqual([0, 1]);
  });

  it("falls back to empty placeholders without data", () => {
    const fields = new FieldMap({});
    expect(fields.count).toBe(0);
    expect(fields.get("rho").data).toHaveLength(0);
    expect(fields.get("r").data).toHaveLength(0);
  });

  it("rejects frames without mandatory fields", () => {
    expect(() => new FieldMap({ rho: new Float32Array(2) })).toThrow(
      "missing the position field",
    );
    expect(() => new FieldMap({ r: new Float32Array(4) })).toThrow(
      "missing the density field",
    );
  });

  it("rejects fields of unsupported sizes", () => {
    expect(
      () =>
        new FieldMap({
          rho: new Float32Array(2),
          r: new Float32Array(4),
          bad: new Float32Array(3),
        }),
    ).toThrow("unsupported size");
  });
});

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

describe("field modifiers", () => {
  const fields = makeFieldMap();

  it("defaults by rank", () => {
    expect(fieldModifierDefault(fields.get("rho"))).toBe(0);
    expect(fieldModifierDefault(fields.get("v"))).toBe("magnitude");
    expect(fieldModifierDefault(fields.get("S"))).toBe("determinant");
  });

  it("validates modifiers against the field shape", () => {
    expect(isValidFieldModifier(fields.get("v"), "magnitude")).toBe(true);
    expect(isValidFieldModifier(fields.get("S"), "magnitude")).toBe(false);
    expect(isValidFieldModifier(fields.get("S"), "determinant")).toBe(true);
    expect(isValidFieldModifier(fields.get("rho"), 0)).toBe(true);
    expect(isValidFieldModifier(fields.get("v"), 1)).toBe(true);
    expect(isValidFieldModifier(fields.get("v"), 2)).toBe(false);
    expect(isValidFieldModifier(fields.get("S"), 3)).toBe(true);
  });

  it("formats display titles", () => {
    expect(fieldModifierToString(fields.get("rho"), 0)).toBe("rho");
    expect(fieldModifierToString(fields.get("v"), "magnitude")).toBe("|v|");
    expect(fieldModifierToString(fields.get("v"), 1)).toBe("v.y");
    expect(fieldModifierToString(fields.get("S"), "determinant")).toBe(
      "det(S)",
    );
    expect(fieldModifierToString(fields.get("S"), 2)).toBe("S.yx");
  });
});

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
