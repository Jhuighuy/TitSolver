/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { z } from "zod";

import type { Spec } from "~/shared/spec";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export interface TreeMap {
  [key: string]: Tree;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export type Tree = null | boolean | number | string | Tree[] | TreeMap;

export const treeSchema: z.ZodType<Tree> = z.lazy(() =>
  z.union([
    z.null(),
    z.boolean(),
    z.number(),
    z.string(),
    z.array(treeSchema),
    z.record(z.string(), treeSchema),
  ]),
);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export function isTreeMap(value: Tree): value is TreeMap {
  return typeof value === "object" && value !== null && !Array.isArray(value);
}

export function treeType(value: Tree): string {
  if (value === null) return "null";
  if (Array.isArray(value)) return "array";
  if (Number.isInteger(value)) return "integer";
  return typeof value;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export interface NormalizeTreeResult {
  tree: Tree;
  isComplete: boolean;
  warnings: string[];
}

export function normalizeTree(
  spec: Spec,
  tree: Tree,
  path = "",
): NormalizeTreeResult {
  const name = path || "root";

  switch (spec.type) {
    case "bool":
      if (typeof tree === "boolean") {
        return { tree, isComplete: true, warnings: [] };
      }

      return {
        tree: spec.default,
        isComplete: true,
        warnings:
          tree === null
            ? []
            : [`Property "${name}": expected boolean, got ${treeType(tree)}.`],
      };

    case "int": {
      if (typeof tree === "number") {
        return {
          tree: Math.round(
            Math.max(spec.min ?? -Infinity, Math.min(spec.max ?? Infinity, tree)),
          ),
          isComplete: true,
          warnings: [],
        };
      }

      if (spec.default !== undefined) {
        return {
          tree: spec.default,
          isComplete: true,
          warnings:
            tree === null
              ? []
              : [`Property "${name}": expected integer, got ${treeType(tree)}.`],
        };
      }

      return {
        tree: null,
        isComplete: false,
        warnings:
          tree === null
            ? []
            : [`Property "${name}": expected integer, got ${treeType(tree)}.`],
      };
    }

    case "real": {
      if (typeof tree === "number") {
        return {
          tree: Math.max(spec.min ?? -Infinity, Math.min(spec.max ?? Infinity, tree)),
          isComplete: true,
          warnings: [],
        };
      }

      if (spec.default !== undefined) {
        return {
          tree: spec.default,
          isComplete: true,
          warnings:
            tree === null
              ? []
              : [
                  `Property "${name}": expected real number, got ${treeType(tree)}.`,
                ],
        };
      }

      return {
        tree: null,
        isComplete: false,
        warnings:
          tree === null
            ? []
            : [`Property "${name}": expected real number, got ${treeType(tree)}.`],
      };
    }

    case "string":
      if (typeof tree === "string") {
        return { tree, isComplete: true, warnings: [] };
      }

      if (spec.default !== undefined) {
        return {
          tree: spec.default,
          isComplete: true,
          warnings:
            tree === null
              ? []
              : [`Property "${name}": expected string, got ${treeType(tree)}.`],
        };
      }

      return {
        tree: null,
        isComplete: false,
        warnings:
          tree === null
            ? []
            : [`Property "${name}": expected string, got ${treeType(tree)}.`],
      };

    case "enum": {
      const isValidOption = (value: Tree): value is string =>
        typeof value === "string" && spec.options.some((option) => option.id === value);

      if (isValidOption(tree)) {
        return { tree, isComplete: true, warnings: [] };
      }

      if (spec.default !== undefined) {
        return {
          tree: spec.default,
          isComplete: true,
          warnings:
            tree === null
              ? []
              : [
                  `Property "${name}": invalid enum value ${JSON.stringify(tree)}.`,
                ],
        };
      }

      return {
        tree: null,
        isComplete: false,
        warnings:
          tree === null
            ? []
            : [`Property "${name}": invalid enum value ${JSON.stringify(tree)}.`],
      };
    }

    case "array": {
      if (!Array.isArray(tree)) {
        return {
          tree: [],
          isComplete: true,
          warnings:
            tree === null
              ? []
              : [`Property "${name}": expected array, got ${treeType(tree)}.`],
        };
      }

      const items = tree.map((item, index) =>
        normalizeTree(spec.item, item, `${name}[${String(index)}]`),
      );

      return {
        tree: items.map((item) => item.tree),
        isComplete: items.every((item) => item.isComplete),
        warnings: items.flatMap((item) => item.warnings),
      };
    }

    case "record": {
      const map = isTreeMap(tree) ? tree : {};
      const fields = spec.fields.map((field) => {
        const fieldPath = path ? `${path}.${field.id}` : field.id;
        return [field.id, normalizeTree(field.spec, map[field.id] ?? null, fieldPath)] as const;
      });

      return {
        tree: Object.fromEntries(fields.map(([fieldId, field]) => [fieldId, field.tree])),
        isComplete: fields.every(([, field]) => field.isComplete),
        warnings: [
          ...(tree === null || isTreeMap(tree)
            ? []
            : [`Property "${name}": expected record, got ${treeType(tree)}.`]),
          ...fields.flatMap(([, field]) => field.warnings),
        ],
      };
    }

    case "variant": {
      const map = isTreeMap(tree) ? tree : {};
      const rawActive = map._active;
      const hasRawActive = Object.hasOwn(map, "_active");
      const fallbackActiveId = spec.default ?? spec.options[0]?.id;
      const activeId =
        typeof rawActive === "string" &&
        spec.options.some((option) => option.id === rawActive)
          ? rawActive
          : fallbackActiveId;

      const normalizedOptions = spec.options.flatMap((option) => {
        if (option.id !== activeId && !Object.hasOwn(map, option.id)) {
          return [];
        }

        return [
          [option.id, normalizeTree(option.spec, map[option.id] ?? null, `${name}.${option.id}`)] as const,
        ];
      });
      const activeOption = normalizedOptions.find(([optionId]) => optionId === activeId)?.[1];

      return {
        tree: {
          _active: activeId,
          ...Object.fromEntries(
            normalizedOptions.map(([optionId, option]) => [optionId, option.tree]),
          ),
        },
        isComplete: activeOption === undefined ? false : activeOption.isComplete,
        warnings: [
          ...(tree === null || isTreeMap(tree)
            ? []
            : [`Property "${name}": expected variant, got ${treeType(tree)}.`]),
          ...(typeof rawActive === "string" &&
          spec.options.some((option) => option.id === rawActive)
            ? []
            : !hasRawActive || rawActive === null
              ? []
              : [
                  `Variant "${name}": unknown active option ${JSON.stringify(rawActive)}.`,
                ]),
          ...normalizedOptions.flatMap(([, option]) => option.warnings),
        ],
      };
    }
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export function treeEquals(a: Tree, b: Tree): boolean {
  if (a === b) return true;

  if (typeof a !== typeof b) return false;
  if (a === null || b === null) return false;

  if (Array.isArray(a) || Array.isArray(b)) {
    if (!Array.isArray(a) || !Array.isArray(b) || a.length !== b.length) {
      return false;
    }

    return a.every((value, index) => treeEquals(value, b[index] ?? null));
  }

  if (typeof a !== "object" || typeof b !== "object") {
    return false;
  }

  const aKeys = Object.keys(a);
  const bKeys = Object.keys(b);
  if (aKeys.length !== bKeys.length) return false;
  if (!aKeys.every((key) => Object.hasOwn(b, key))) return false;

  return aKeys.every((key) => treeEquals(a[key] ?? null, b[key] ?? null));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
