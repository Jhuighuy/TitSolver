/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export type PropTreeJSON =
  | null
  | boolean
  | number
  | string
  | PropTreeJSON[]
  | { [key: string]: PropTreeJSON };

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export type PropIssueCode =
  | "missing_required"
  | "invalid_type"
  | "invalid_value"
  | "below_minimum"
  | "above_maximum"
  | "unknown_field"
  | "unknown_option"
  | "duplicate_symbol"
  | "unresolved_ref";

export interface PropIssue {
  code: PropIssueCode;
  path: string;
  message: string;
}

export type PropNamespaceTable = {
  [namespace: string]: { [symbol: string]: string };
};

export interface PropMaterializationResult {
  tree: PropTreeObject;
  issues: PropIssue[];
  namespaceTable: PropNamespaceTable;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export interface PropTreeObject {
  toJSON(): PropTreeJSON;
  toJSONText(): string;
  saveToFile(path: string): void;
}

export interface PropTreeConstructor {
  fromJSON(value: PropTreeJSON): PropTreeObject;
  fromJSONText(text: string): PropTreeObject;
  fromFile(path: string): PropTreeObject;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export interface PropSpec {
  toJSON(): unknown;
  materialize(tree: PropTreeObject): PropMaterializationResult;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
