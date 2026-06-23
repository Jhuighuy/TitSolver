/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { z } from "zod";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Tree
//

export type Tree = null | boolean | number | string | Tree[] | TreeMap;

export interface TreeMap {
  [key: string]: Tree;
}

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

export function isTreeMap(value: Tree): value is TreeMap {
  return typeof value === "object" && value !== null && !Array.isArray(value);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Spec
//

const boolSpecSchema = z.object({
  type: z.literal("bool"),
  default: z.boolean(),
});

const numberSpecSchema = z.union([
  z.object({
    type: z.literal("int"),
    min: z.int().optional(),
    max: z.int().optional(),
    default: z.int().optional(),
  }),
  z.object({
    type: z.literal("real"),
    min: z.number().optional(),
    max: z.number().optional(),
    default: z.number().optional(),
    unit: z.string().optional(),
  }),
]);

const stringSpecSchema = z.object({
  type: z.literal("string"),
  default: z.string().optional(),
});

const enumOptionSchema = z.object({
  id: z.string(),
  name: z.string(),
});

const enumSpecSchema = z.object({
  type: z.literal("enum"),
  options: z.array(enumOptionSchema),
  default: z.string().optional(),
});

const arraySpecSchema = z.object({
  type: z.literal("array"),
  size: z.int().nonnegative().optional(),
  get item() {
    return specSchema;
  },
});

const recordFieldSpecSchema = z.object({
  id: z.string(),
  name: z.string(),
  get spec() {
    return specSchema;
  },
});

const recordSpecSchema = z.object({
  type: z.literal("record"),
  get fields() {
    return z.array(recordFieldSpecSchema);
  },
});

const variantOptionSpecSchema = z.object({
  id: z.string(),
  name: z.string(),
  get spec() {
    return specSchema;
  },
});

const variantSpecSchema = z.object({
  type: z.literal("variant"),
  get options() {
    return z.array(variantOptionSpecSchema);
  },
  default: z.string().optional(),
});

const symbolSpecSchema = z.object({
  type: z.literal("symbol"),
  namespace: z.string(),
});

const refSpecSchema = z.object({
  type: z.literal("ref"),
  namespace: z.string(),
});

export const specSchema = z.union([
  boolSpecSchema,
  numberSpecSchema,
  stringSpecSchema,
  enumSpecSchema,
  arraySpecSchema,
  recordSpecSchema,
  variantSpecSchema,
  symbolSpecSchema,
  refSpecSchema,
]);

export type BoolSpec = z.infer<typeof boolSpecSchema>;
export type NumberSpec = z.infer<typeof numberSpecSchema>;
export type StringSpec = z.infer<typeof stringSpecSchema>;
export type EnumSpec = z.infer<typeof enumSpecSchema>;
export type ArraySpec = z.infer<typeof arraySpecSchema>;
export type RecordSpec = z.infer<typeof recordSpecSchema>;
export type VariantSpec = z.infer<typeof variantSpecSchema>;
export type SymbolSpec = z.infer<typeof symbolSpecSchema>;
export type RefSpec = z.infer<typeof refSpecSchema>;
export type Spec = z.infer<typeof specSchema>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Document
//

const issueSchema = z.object({
  code: z.union([
    z.literal("missing_required"),
    z.literal("invalid_type"),
    z.literal("invalid_value"),
    z.literal("below_minimum"),
    z.literal("above_maximum"),
    z.literal("unknown_field"),
    z.literal("unknown_option"),
    z.literal("duplicate_symbol"),
    z.literal("unresolved_ref"),
  ]),
  path: z.string(),
  message: z.string(),
});

const namespaceTableSchema = z.record(
  z.string(),
  z.record(z.string(), z.string()),
);

export const propertyDocumentSchema = z.object({
  spec: specSchema,
  tree: treeSchema,
  issues: z.array(issueSchema),
  namespaceTable: namespaceTableSchema,
  revision: z.int().nonnegative(),
  dirty: z.boolean(),
});

export type PropertyIssue = z.infer<typeof issueSchema>;
export type NamespaceTable = z.infer<typeof namespaceTableSchema>;
export type PropertyDocument = z.infer<typeof propertyDocumentSchema>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
