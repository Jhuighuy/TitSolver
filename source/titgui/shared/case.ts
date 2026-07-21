/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { z } from "zod";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
 * Name of the case file inside a case directory.
 */
export const CASE_FILE_NAME = "case.yaml";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
 * JSON representation of a property tree.
 */
export type TreeJson =
  | null
  | boolean
  | number
  | string
  | TreeJson[]
  | { [key: string]: TreeJson };

export const treeJsonSchema: z.ZodType<TreeJson> = z.lazy(() =>
  z.union([
    z.null(),
    z.boolean(),
    z.number(),
    z.string(),
    z.array(treeJsonSchema),
    z.record(z.string(), treeJsonSchema),
  ]),
);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
 * JSON representation of a property specification.
 */
export type SpecJson =
  | { type: "bool"; default: boolean }
  | { type: "int"; min?: number; max?: number; default?: number }
  | {
      type: "real";
      min?: number;
      max?: number;
      default?: number;
      unit?: string;
    }
  | { type: "string"; default?: string }
  | { type: "enum"; options: { id: string; name: string }[]; default?: string }
  | { type: "array"; size?: number; item: SpecJson }
  | { type: "record"; fields: { id: string; name: string; spec: SpecJson }[] }
  | {
      type: "variant";
      options: { id: string; name: string; spec: SpecJson }[];
      default?: string;
    }
  | { type: "symbol"; namespace: string }
  | { type: "ref"; namespace: string };

export const specJsonSchema: z.ZodType<SpecJson> = z.lazy(() =>
  z.discriminatedUnion("type", [
    z.object({
      type: z.literal("bool"),
      default: z.boolean(),
    }),
    z.object({
      type: z.literal("int"),
      min: z.number().optional(),
      max: z.number().optional(),
      default: z.number().optional(),
    }),
    z.object({
      type: z.literal("real"),
      min: z.number().optional(),
      max: z.number().optional(),
      default: z.number().optional(),
      unit: z.string().optional(),
    }),
    z.object({
      type: z.literal("string"),
      default: z.string().optional(),
    }),
    z.object({
      type: z.literal("enum"),
      options: z.array(z.object({ id: z.string(), name: z.string() })),
      default: z.string().optional(),
    }),
    z.object({
      type: z.literal("array"),
      size: z.int().optional(),
      item: specJsonSchema,
    }),
    z.object({
      type: z.literal("record"),
      fields: z.array(
        z.object({ id: z.string(), name: z.string(), spec: specJsonSchema }),
      ),
    }),
    z.object({
      type: z.literal("variant"),
      options: z.array(
        z.object({ id: z.string(), name: z.string(), spec: specJsonSchema }),
      ),
      default: z.string().optional(),
    }),
    z.object({
      type: z.literal("symbol"),
      namespace: z.string(),
    }),
    z.object({
      type: z.literal("ref"),
      namespace: z.string(),
    }),
  ]),
);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
 * Stable machine-readable validation issue code.
 */
export const issueCodeSchema = z.union([
  z.literal("missing_required"),
  z.literal("invalid_type"),
  z.literal("invalid_value"),
  z.literal("below_minimum"),
  z.literal("above_maximum"),
  z.literal("unknown_field"),
  z.literal("unknown_option"),
  z.literal("duplicate_symbol"),
  z.literal("unresolved_ref"),
]);

export type IssueCode = z.infer<typeof issueCodeSchema>;

/**
 * Validation issue reported by materialization.
 */
export const issueSchema = z.object({
  code: issueCodeSchema,

  /** Path to the affected tree node. */
  path: z.string(),

  /** User-facing issue message. */
  message: z.string(),
});

export type Issue = z.infer<typeof issueSchema>;

/**
 * Symbol table, grouped by namespace: namespace → symbol → tree path.
 */
export const namespaceTableSchema = z.record(
  z.string(),
  z.record(z.string(), z.string()),
);

export type NamespaceTable = z.infer<typeof namespaceTableSchema>;

/**
 * Result of materializing a case tree against the case specification.
 */
export const materializedCaseSchema = z.object({
  /** Normalized tree: defaults filled in, values coerced. */
  tree: treeJsonSchema,
  issues: z.array(issueSchema),
  namespaces: namespaceTableSchema,
});

export type MaterializedCase = z.infer<typeof materializedCaseSchema>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
 * The case document: the authored tree together with its materialization.
 * Pushed to the renderer whenever the tree changes.
 */
export const caseDocumentSchema = z.object({
  /**
   * Revision of the authored tree. Edits must carry the revision they were
   * based on; stale edits are rejected.
   */
  revision: z.int().nonnegative(),

  /** The authored tree: exactly what the user set, as stored on disk. */
  authored: treeJsonSchema,

  /** Materialization of the authored tree. */
  materialized: materializedCaseSchema,
});

export type CaseDocument = z.infer<typeof caseDocumentSchema>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
 * State of the open case, or `null` when no case is open.
 */
export const caseStateSchema = z
  .object({
    /** Case directory. */
    dir: z.string(),

    /** Display name (the directory basename). */
    name: z.string(),

    /** Whether the case has unsaved changes. */
    dirty: z.boolean(),
  })
  .nullable();

export type CaseState = z.infer<typeof caseStateSchema>;

/**
 * Entry of the recently opened cases list.
 */
export const recentCaseSchema = z.object({
  dir: z.string(),
  name: z.string(),

  /** Epoch milliseconds of the last open. */
  lastOpenedAt: z.number(),
});

export type RecentCase = z.infer<typeof recentCaseSchema>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
 * Convert a tree path to the string form used by validation issues,
 * e.g. `["fluid", "density"]` → `"/fluid/density"`.
 */
export function treePathToString(path: readonly string[]) {
  return `/${path.join("/")}`;
}

/**
 * Get the value at the given path of a tree, or `undefined` when any map
 * along the path is missing the key.
 */
export function treeGetAt(
  tree: TreeJson,
  path: readonly string[],
): TreeJson | undefined {
  let node: TreeJson | undefined = tree;
  for (const key of path) {
    if (node === null || typeof node !== "object" || Array.isArray(node)) {
      return undefined;
    }
    node = key in node ? node[key] : undefined;
  }
  return node;
}

/**
 * Immutably set the value at the given path of a tree, materializing any
 * missing intermediate maps. Non-map nodes along the path are replaced.
 */
export function treeSetAt(
  tree: TreeJson,
  path: readonly string[],
  value: TreeJson,
): TreeJson {
  if (path.length === 0) return value;

  const [key, ...rest] = path;
  const map =
    tree !== null && typeof tree === "object" && !Array.isArray(tree)
      ? tree
      : {};
  return { ...map, [key]: treeSetAt(map[key] ?? null, rest, value) };
}

/**
 * Immutably delete the value at the given path of a tree, pruning maps that
 * become empty. Deleting a missing path returns the tree unchanged.
 */
export function treeDeleteAt(
  tree: TreeJson,
  path: readonly string[],
): TreeJson {
  if (path.length === 0) return null;
  if (tree === null || typeof tree !== "object" || Array.isArray(tree)) {
    return tree;
  }

  const [key, ...rest] = path;
  if (!(key in tree)) return tree;

  const map = { ...tree };
  if (rest.length === 0) {
    delete map[key];
    return map;
  }

  const child = treeDeleteAt(map[key], rest);
  if (
    child !== null &&
    typeof child === "object" &&
    !Array.isArray(child) &&
    Object.keys(child).length === 0
  ) {
    delete map[key];
  } else {
    map[key] = child;
  }
  return map;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
