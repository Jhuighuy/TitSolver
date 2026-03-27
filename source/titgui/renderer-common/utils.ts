/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { assert } from "~/shared/utils";

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

  const bytes = Uint8Array.from(atob(b64), (c) => c.codePointAt(0) ?? 0);
  assert(
    bytes.byteLength % ctor.BYTES_PER_ELEMENT === 0,
    `Byte length ${bytes.byteLength} is not a multiple of ${ctor.BYTES_PER_ELEMENT} for type ${type}`,
  );

  const vals = new ctor(
    bytes.buffer,
    bytes.byteOffset,
    bytes.byteLength / ctor.BYTES_PER_ELEMENT,
  );

  // Convert `bigint` arrays to number arrays.
  return vals instanceof BigInt64Array || vals instanceof BigUint64Array
    ? Float64Array.from(vals, Number)
    : vals;
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

/**
 * State updater type.
 */
export type SetStateAction<T> = T | ((prev: T) => T);

/**
 * Type guard for state updater functions.
 */
export function isStateUpdater<T>(
  next: SetStateAction<T>,
): next is (prev: T) => T {
  return typeof next === "function";
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
 * Download a file with the given name and content.
 */
export function downloadFile(fileName: string, url: string) {
  const linkElement = document.createElement("a");
  linkElement.href = url;
  linkElement.download = fileName;
  linkElement.click();
}

/**
 * Download a blob with the given name and content.
 */
export function downloadBlob(fileName: string, blob: Blob) {
  const url = URL.createObjectURL(blob);
  downloadFile(fileName, url);
  URL.revokeObjectURL(url);
}

/**
 * Download a text file with the given name and content.
 */
export function downloadText(fileName: string, text: string) {
  downloadBlob(fileName, new Blob([text], { type: "text/plain" }));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
