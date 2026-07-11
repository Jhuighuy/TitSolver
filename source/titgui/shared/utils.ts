/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
 * Assert that the condition is true. Reserved for programmer invariants: a
 * failed assertion is a bug, never an expected runtime state.
 */
export function assert(
  condition: unknown,
  message?: string,
): asserts condition {
  if (!condition) throw new Error(message ?? "Assertion failed.");
}

/**
 * Ensure that a runtime condition holds. Unlike `assert`, this guards
 * conditions caused by external data, files, or processes — failures are
 * expected states that callers must be prepared to handle.
 */
export function ensure(condition: unknown, message: string): asserts condition {
  if (!condition) throw new Error(message);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
 * Generate an array of numbers from `0` to `n - 1`.
 */
export function iota(n: number): number[] {
  return Array.from({ length: n }, (_, i) => i);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
 * Extract an array from a single item or an array.
 */
export function items<T>(arg: T | T[]): T[] {
  return Array.isArray(arg) ? arg : [arg];
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
