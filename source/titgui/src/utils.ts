/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { type ClassValue, clsx } from "clsx";
import { twMerge } from "tailwind-merge";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
 * Assert that the condition is true.
 */
export function assert(
  condition: unknown,
  message?: string
): asserts condition {
  if (!condition) throw new Error(message ?? "Assertion failed.");
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
 * Combine class names.
 */
export function cn(...inputs: ClassValue[]): string {
  return twMerge(clsx(inputs));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
 * Expand a 2D array to a 3D array.
 */
export function expandTo3D(input: ArrayLike<number>): Float32Array {
  assert(input.length % 2 === 0);

  const count = input.length / 2;
  const result = new Float32Array(count * 3);

  for (let i = 0; i < count; i++) {
    result[3 * i + 0] = input[2 * i + 0];
    result[3 * i + 1] = input[2 * i + 1];
    result[3 * i + 2] = 0;
  }

  return result;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
 * Decode a Base64-encoded string to a typed array.
 *
 * Note: 64-bit integer types are converted to `Float64Array`, which may lose
 *       precision for values outside the safe integer range.
 */
export function decodeBase64(b64: string, type: string) {
  const typeMap = {
    int8_t: Int8Array,
    uint8_t: Uint8Array,
    int16_t: Int16Array,
    uint16_t: Uint16Array,
    int32_t: Int32Array,
    uint32_t: Uint32Array,
    int64_t: BigInt64Array,
    uint64_t: BigUint64Array,
    float32_t: Float32Array,
    float64_t: Float64Array,
  };

  assert(type in typeMap);
  const ctor = typeMap[type as keyof typeof typeMap];

  const bytes = Uint8Array.from(atob(b64), (c) => c.charCodeAt(0));
  assert(
    bytes.byteLength % ctor.BYTES_PER_ELEMENT === 0,
    `Byte length ${bytes.byteLength} is not a multiple of ${ctor.BYTES_PER_ELEMENT} for type ${type}`
  );

  const vals = new ctor(
    bytes.buffer,
    bytes.byteOffset,
    bytes.byteLength / ctor.BYTES_PER_ELEMENT
  );

  // Convert `bigint` arrays to number arrays.
  return vals instanceof BigInt64Array || vals instanceof BigUint64Array
    ? Float64Array.from(vals, Number)
    : vals;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
 * Calculate minimum and maximum values of an array.
 */
export function minMax(arr: Iterable<number>): [number, number] {
  let empty = true;
  let min = Number.POSITIVE_INFINITY;
  let max = Number.NEGATIVE_INFINITY;

  for (const val of arr) {
    empty = false;
    min = Math.min(min, val);
    max = Math.max(max, val);
  }

  assert(!empty);
  return [min, max];
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
 * Convert a number representing color to a CSS color string.
 */
export function toCSSColor(num: number): string {
  assert(0 <= num && num <= 0xffffff);
  return `#${num.toString(16).padStart(6, "0")}`;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
