/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { Vector3 } from "three";

import { assert } from "~/utils";
import type { ColorMapName } from "~/visual/color-map";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

type BaseProperty = {
  name: string;
  unit?: string;
  disabled?: boolean;
};

export type TypedProperty<T> = BaseProperty & {
  value: T;
  setValue: (value: T) => void;
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export type BoolProperty = TypedProperty<boolean> & {
  type: "bool";
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export type EnumProperty = TypedProperty<string> & {
  type: "enum";
  options: string[];
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export type ColorMapProperty = TypedProperty<ColorMapName> & {
  type: "colormap";
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export type StringProperty = TypedProperty<string> & {
  type: "string";
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export type IntProperty = TypedProperty<number> & {
  type: "int";
  min?: number;
  max?: number;
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export type FloatProperty = TypedProperty<number> & {
  type: "float";
  min?: number;
  max?: number;
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export type HandleProperty = BaseProperty & {
  type: "handle";
  target: Property;
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

type NestedProperty = BaseProperty & {
  children: {
    props: Property;
  }[];
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export type ArrayProperty = NestedProperty & {
  type: "array";
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export type RecordProperty = (NestedProperty | TypedProperty<Vector3>) & {
  type: "record";
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export type ScalarProperty =
  | BoolProperty
  | EnumProperty
  | ColorMapProperty
  | StringProperty
  | IntProperty
  | FloatProperty
  | HandleProperty;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export type Property = ScalarProperty | RecordProperty | ArrayProperty;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export function getNestedItems(p: ArrayProperty | RecordProperty): Property[] {
  // Base case: simple nested component.
  if ("children" in p) {
    return p.children.map((child) => child.props);
  }

  // `Vector3` record.
  if (p.value instanceof Vector3) {
    return ["x", "y", "z"].map((coord, index) => {
      return {
        type: "float",
        unit: p.unit,
        name: coord,
        value: p.value.getComponent(index),
        setValue: (v) => p.setValue(p.value.clone().setComponent(index, v)),
      };
    });
  }

  assert(false);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
