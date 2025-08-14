/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { z } from "zod";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

const boolSpecSchema = z.object({
  type: z.literal("bool"),
  default: z.boolean().optional(),
});

const intSpecSchema = z.object({
  type: z.literal("int"),
  default: z.number().int().optional(),
  min: z.number().int().optional(),
  max: z.number().int().optional(),
});

const realSpecSchema = z.object({
  type: z.literal("real"),
  default: z.number().optional(),
  min: z.number().optional(),
  max: z.number().optional(),
});

const stringSpecSchema = z.object({
  type: z.literal("string"),
  default: z.string().optional(),
});

const enumSpecSchema = z.object({
  type: z.literal("enum"),
  default: z.string().optional(),
  options: z.array(
    z.object({
      id: z.string(),
      name: z.string(),
    }),
  ),
});

export const propertySpecSchema: z.ZodType<PropertySpec> = z.lazy(() =>
  z.union([
    boolSpecSchema,
    intSpecSchema,
    realSpecSchema,
    stringSpecSchema,
    enumSpecSchema,
    z.object({
      type: z.literal("record"),
      fields: z.array(fieldSpecSchema),
    }),
    z.object({
      type: z.literal("array"),
      items: propertySpecSchema,
    }),
    z.object({
      type: z.literal("variant"),
      default: z.string().optional(),
      options: z.record(
        z.string(),
        z.object({
          name: z.string(),
          spec: propertySpecSchema,
        }),
      ),
    }),
  ]),
);

export const fieldSpecSchema = z.object({
  id: z.string(),
  name: z.string(),
  spec: propertySpecSchema,
});

export const rootSpecSchema = z.object({
  type: z.literal("record"),
  fields: z.array(fieldSpecSchema),
});

export type BoolSpec = z.infer<typeof boolSpecSchema>;
export type IntSpec = z.infer<typeof intSpecSchema>;
export type RealSpec = z.infer<typeof realSpecSchema>;
export type StringSpec = z.infer<typeof stringSpecSchema>;
export type EnumSpec = z.infer<typeof enumSpecSchema>;
export type FieldSpec = z.infer<typeof fieldSpecSchema>;
export type RootSpec = z.infer<typeof rootSpecSchema>;

export type PropertySpec =
  | BoolSpec
  | IntSpec
  | RealSpec
  | StringSpec
  | EnumSpec
  | {
      type: "record";
      fields: FieldSpec[];
    }
  | {
      type: "array";
      items: PropertySpec;
    }
  | {
      type: "variant";
      default?: string;
      options: Record<
        string,
        {
          name: string;
          spec: PropertySpec;
        }
      >;
    };

export type PropertyTree =
  | boolean
  | number
  | string
  | PropertyRecord
  | PropertyTree[];

export type PropertyRecord = {
  [key: string]: PropertyTree;
};

export const propertyTreeSchema: z.ZodType<PropertyTree> = z.lazy(() =>
  z.union([
    z.boolean(),
    z.number(),
    z.string(),
    z.array(propertyTreeSchema),
    z.record(z.string(), propertyTreeSchema),
  ]),
);

export const solverConfigSchema = z.object({
  spec: rootSpecSchema,
  tree: z.record(z.string(), propertyTreeSchema),
});

export type SolverConfig = z.infer<typeof solverConfigSchema>;

export type NormalizationIssue = {
  path: string;
  expected: string;
  actual: string;
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export function createDefaultValue(spec: PropertySpec): PropertyTree {
  switch (spec.type) {
    case "bool":
      return spec.default ?? false;
    case "int":
    case "real":
      return spec.default ?? 0;
    case "string":
      return spec.default ?? "";
    case "enum":
      return spec.default ?? spec.options[0]?.id ?? "";
    case "record":
      return Object.fromEntries(
        spec.fields.map((field) => [field.id, createDefaultValue(field.spec)]),
      );
    case "array":
      return [];
    case "variant": {
      const active = spec.default ?? Object.keys(spec.options)[0] ?? "";
      const option = spec.options[active];
      return {
        _active: active,
        ...(option ? { [active]: createDefaultValue(option.spec) } : {}),
      };
    }
  }
}

export function hasDefaultValue(spec: PropertySpec): boolean {
  switch (spec.type) {
    case "bool":
    case "int":
    case "real":
    case "string":
    case "enum":
      return spec.default !== undefined;
    case "record":
      return spec.fields.some((field) => hasDefaultValue(field.spec));
    case "array":
      return true;
    case "variant":
      return (
        spec.default !== undefined ||
        Object.keys(spec.options).length > 0 ||
        Object.values(spec.options).some((option) =>
          hasDefaultValue(option.spec),
        )
      );
  }
}

export function isPropertyTreeCompatible(
  spec: PropertySpec,
  value: unknown,
): value is PropertyTree {
  switch (spec.type) {
    case "bool":
      return typeof value === "boolean";
    case "int":
      return typeof value === "number" && Number.isInteger(value);
    case "real":
      return typeof value === "number";
    case "string":
      return typeof value === "string";
    case "enum":
      return (
        typeof value === "string" &&
        spec.options.some((option) => option.id === value)
      );
    case "record":
      return (
        isPlainObject(value) &&
        spec.fields.every((field) =>
          isPropertyTreeCompatible(field.spec, value[field.id]),
        )
      );
    case "array":
      return (
        Array.isArray(value) &&
        value.every((item) => isPropertyTreeCompatible(spec.items, item))
      );
    case "variant": {
      if (!isPlainObject(value) || typeof value._active !== "string")
        return false;
      const option = spec.options[value._active];
      if (!option) return false;
      return isPropertyTreeCompatible(option.spec, value[value._active]);
    }
  }
}

// ---- Normalization. ---------------------------------------------------------

export function normalizePropertyTree(
  spec: PropertySpec,
  value: unknown,
  path = "",
): {
  value: PropertyTree;
  issues: NormalizationIssue[];
} {
  switch (spec.type) {
    case "bool":
      return normalizeScalar(spec, value, path, "bool");
    case "int":
      return normalizeScalar(spec, value, path, "int");
    case "real":
      return normalizeScalar(spec, value, path, "real");
    case "string":
      return normalizeScalar(spec, value, path, "string");
    case "enum":
      return normalizeScalar(spec, value, path, "enum");
    case "record": {
      if (!isPlainObject(value)) {
        return invalidValue(spec, value, path, "record");
      }

      const issues: NormalizationIssue[] = [];
      const normalized = Object.fromEntries(
        spec.fields.map((field) => {
          const child = normalizePropertyTree(
            field.spec,
            value[field.id],
            joinPath(path, field.id),
          );
          issues.push(...child.issues);
          return [field.id, child.value];
        }),
      );

      return { value: normalized, issues };
    }
    case "array": {
      if (!Array.isArray(value)) {
        return invalidValue(spec, value, path, "array");
      }

      const issues: NormalizationIssue[] = [];
      const normalized = value.map((item, index) => {
        const child = normalizePropertyTree(
          spec.items,
          item,
          joinPath(path, `[${index}]`),
        );
        issues.push(...child.issues);
        return child.value;
      });

      return { value: normalized, issues };
    }
    case "variant": {
      if (!isPlainObject(value) || typeof value._active !== "string") {
        return invalidValue(spec, value, path, "variant");
      }

      const option = spec.options[value._active];
      if (!option) {
        return invalidValue(spec, value, path, "variant");
      }

      const child = normalizePropertyTree(
        option.spec,
        value[value._active],
        joinPath(path, value._active),
      );
      return {
        value: {
          _active: value._active,
          [value._active]: child.value,
        },
        issues: child.issues,
      };
    }
  }
}

export function normalizeRootTree(
  spec: RootSpec,
  value: unknown,
): {
  tree: PropertyRecord;
  issues: NormalizationIssue[];
} {
  const normalized = normalizePropertyTree(spec, value);
  return {
    tree: normalized.value as PropertyRecord,
    issues: normalized.issues,
  };
}

function isPlainObject(value: unknown): value is Record<string, unknown> {
  return typeof value === "object" && value !== null && !Array.isArray(value);
}

// ---- Helpers. ---------------------------------------------------------------

function normalizeScalar(
  spec: PropertySpec,
  value: unknown,
  path: string,
  expected: string,
): {
  value: PropertyTree;
  issues: NormalizationIssue[];
} {
  if (isPropertyTreeCompatible(spec, value)) {
    return { value, issues: [] };
  }
  return invalidValue(spec, value, path, expected);
}

function invalidValue(
  spec: PropertySpec,
  value: unknown,
  path: string,
  expected: string,
): {
  value: PropertyTree;
  issues: NormalizationIssue[];
} {
  return {
    value: createDefaultValue(spec),
    issues: [
      {
        path: path || "<root>",
        expected,
        actual: describeValue(value),
      },
    ],
  };
}

function joinPath(base: string, key: string): string {
  if (!base) return key;
  if (key.startsWith("[")) return `${base}${key}`;
  return `${base}.${key}`;
}

function describeValue(value: unknown): string {
  if (Array.isArray(value)) return "array";
  if (value === null) return "null";
  return typeof value;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
