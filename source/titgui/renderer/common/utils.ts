/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { assert } from "~/shared/utils";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
 * Convert a number representing color to a CSS color string.
 */
export function toCSSColor(num: number): string {
  assert(0 <= num && num <= 0xffffff);
  return `#${num.toString(16).padStart(6, "0")}`;
}

/**
 * Resolve any CSS color (including `oklch(...)` and `color-mix(...)`) to
 * normalized RGB components by rasterizing it onto a 1x1 canvas.
 */
export function cssColorToRgb(color: string): [number, number, number] {
  const canvas = document.createElement("canvas");
  canvas.width = 1;
  canvas.height = 1;
  const context = canvas.getContext("2d", { willReadFrequently: true });
  assert(context !== null, "2D canvas is not available.");

  context.fillStyle = color;
  context.fillRect(0, 0, 1, 1);
  const [r, g, b] = context.getImageData(0, 0, 1, 1).data;
  return [r / 255, g / 255, b / 255];
}

/**
 * Resolve a CSS custom property to normalized RGB components.
 */
export function cssVariableToRgb(name: string): [number, number, number] {
  const value = getComputedStyle(document.documentElement).getPropertyValue(
    name,
  );
  assert(value.trim() !== "", `CSS variable '${name}' is not defined.`);
  return cssColorToRgb(value);
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

/**
 * Apply a state update action to the given state.
 */
export function applyStateUpdate<T>(prev: T, next: SetStateAction<T>): T {
  return isStateUpdater(next) ? next(prev) : next;
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

/**
 * Format a timestamp as a time of day, like `12:34:56`.
 */
export function formatTimestamp(timestamp: number): string {
  return new Date(timestamp).toLocaleTimeString([], {
    hour: "2-digit",
    minute: "2-digit",
    second: "2-digit",
    hour12: false,
  });
}

/**
 * Format a duration in milliseconds, like `01:23:45`.
 */
export function formatDuration(durationMs: number): string {
  assert(durationMs >= 0);
  const totalSeconds = Math.floor(durationMs / 1000);
  const hours = Math.floor(totalSeconds / 3600);
  const minutes = Math.floor((totalSeconds % 3600) / 60);
  const seconds = totalSeconds % 60;
  return [hours, minutes, seconds]
    .map((value) => value.toString().padStart(2, "0"))
    .join(":");
}

/**
 * Format a memory size in bytes, like `1.5 GiB`.
 */
export function formatMemorySize(bytes: number): string {
  assert(bytes >= 0);
  if (bytes === 0) return "0 B";

  const units = ["B", "KiB", "MiB", "GiB", "TiB"];

  let value = bytes;
  let unitIndex = 0;
  while (value >= 1024 && unitIndex < units.length - 1) {
    value /= 1024;
    unitIndex += 1;
  }

  const fractionDigits = value >= 100 || unitIndex === 0 ? 0 : 1;
  return `${value.toFixed(fractionDigits)} ${units[unitIndex]}`;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
