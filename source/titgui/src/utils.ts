/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
 * Assert that the condition is true.
 */
export function assert(
  condition: unknown,
  message?: string,
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
 * Extract an array from a single item or an array.
 */
export function items<T>(arg: T | T[]): T[] {
  return Array.isArray(arg) ? arg : [arg];
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

export function formatDuration(ms: number): string {
  const totalSeconds = Math.max(0, Math.floor(ms / 1000));
  const hours = Math.floor(totalSeconds / 3600);
  const minutes = Math.floor((totalSeconds % 3600) / 60);
  const seconds = totalSeconds % 60;
  return [hours, minutes, seconds]
    .map((value) => value.toString().padStart(2, "0"))
    .join(":");
}

export function formatMemorySize(bytes: number): string {
  if (bytes <= 0) return "0 B";

  const units = ["B", "KB", "MB", "GB", "TB"];
  let value = bytes;
  let unitIndex = 0;
  while (value >= 1024 && unitIndex < units.length - 1) {
    value /= 1024;
    unitIndex += 1;
  }

  const digits = value >= 100 || unitIndex === 0 ? 0 : 1;
  return `${value.toFixed(digits)} ${units[unitIndex]}`;
}

export function formatTimestamp(timestamp: number): string {
  return new Date(timestamp).toLocaleTimeString([], {
    hour: "2-digit",
    minute: "2-digit",
    second: "2-digit",
    hour12: false,
  });
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
