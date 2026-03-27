/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import type { TypedArray } from "three";

import {
  matrixDeterminant,
  vectorMagnitude,
} from "~/renderer-common/utils-math";
import { assert, iota } from "~/shared/utils";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export type FieldRank = 0 | 1 | 2;
export type FieldDim = 2 | 3;
export type FieldModifier = "magnitude" | "determinant" | number;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export class Field {
  public readonly name: string;
  public readonly data: TypedArray;
  public readonly rank: FieldRank;
  public readonly dim: FieldDim;

  public constructor(
    name: string,
    data: TypedArray,
    rank: FieldRank,
    dim: FieldDim,
  ) {
    this.name = name;
    this.data = data;
    this.rank = rank;
    this.dim = dim;
    assert(this.data.length % this.dim ** this.rank === 0);
  }

  public get length(): number {
    return this.data.length;
  }

  public get width(): number {
    return this.dim ** this.rank;
  }

  public get count(): number {
    return this.length / this.width;
  }

  public components(index: number): TypedArray {
    const offset = index * this.width;
    return this.data.subarray(offset, offset + this.width);
  }

  public value(index: number, modifier: FieldModifier): number {
    const components = this.components(index);

    switch (modifier) {
      case "magnitude":
        assert(this.rank === 1);
        return vectorMagnitude(components);
      case "determinant":
        assert(this.rank === 2);
        return matrixDeterminant(components);
    }

    assert(typeof modifier === "number");
    assert(0 <= modifier && modifier < this.width);
    return components[modifier];
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export class FieldMap extends Map<string, Field> {
  public readonly count: number;
  public readonly dim: FieldDim;

  public constructor(rawFields: Record<string, TypedArray>) {
    super();

    // If no fields are provided, fallback to empty placeholder data.
    if (Object.keys(rawFields).length === 0) {
      this.count = 0;
      this.dim = 2;
      this.set("rho", new Field("rho", new Float32Array(), 0, this.dim));
      this.set("r", new Field("r", new Float32Array(), 1, this.dim));
      return;
    }

    // Get density field. It must be present.
    assert("rho" in rawFields);
    const rho = rawFields.rho;

    // Get position field. It must be present.
    assert("r" in rawFields);
    const r = rawFields.r;

    // Dimensionality is inferred from density and position fields.
    this.count = rho.length;
    this.dim = (r.length / rho.length) as FieldDim;
    assert(2 <= this.dim && this.dim <= 3);
    assert(r.length === this.count * this.dim);

    // Assign fields.
    for (const [name, data] of Object.entries(rawFields)) {
      const rank = (() => {
        switch (data.length) {
          case this.count:
            return 0;
          case this.count * this.dim:
            return 1;
          case this.count * this.dim ** 2:
            return 2;
        }
        assert(false);
      })();
      this.set(name, new Field(name, data, rank, this.dim));
    }

    // Add virtual fields.
    this.set(
      "id",
      new Field("id", new Float32Array(iota(this.count)), 0, this.dim),
    );
  }

  public override get(fieldName: string) {
    const field = super.get(fieldName);
    assert(field !== undefined);
    return field;
  }
}

export function fieldModifierDefault(field: Field): FieldModifier {
  switch (field.rank) {
    case 0:
      return 0;
    case 1:
      return "magnitude";
    case 2:
      return "determinant";
  }
  assert(false); // Unreachable.
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export function isValidFieldModifier(
  field: Field,
  modifier: FieldModifier,
): boolean {
  if (typeof modifier === "number") {
    assert(Number.isInteger(modifier));
    return 0 <= modifier && modifier < field.width;
  }

  switch (modifier) {
    case "magnitude":
      return field.rank === 1;
    case "determinant":
      return field.rank === 2;
  }

  assert(false); // Unreachable.
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export function fieldModifierToString(
  field: Field,
  modifier: FieldModifier,
): string {
  switch (modifier) {
    case "magnitude":
      assert(field.rank === 1);
      return `|${field.name}|`;
    case "determinant":
      assert(field.rank === 2);
      return `det(${field.name})`;
  }

  assert(typeof modifier === "number");
  assert(0 <= modifier && modifier < field.width);
  const axes = ["x", "y", "z"].slice(0, field.dim);
  switch (field.rank) {
    case 0:
      assert(modifier === 0);
      return field.name;
    case 1:
      return `${field.name}.${axes[modifier]}`;
    case 2: {
      const row = Math.floor(modifier / field.dim);
      const col = modifier % field.dim;
      return `${field.name}.${axes[row]}${axes[col]}`;
    }
  }

  assert(false); // Unreachable.
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
