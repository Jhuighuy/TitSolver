/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { downloadBlob } from "~/utils";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Keep clipboard and export behavior outside the UI so menus and rows can stay
// focused on editor flow instead of browser APIs.

export async function copyJsonToClipboard(value: unknown) {
  try {
    await navigator.clipboard.writeText(JSON.stringify(value, null, 2));
  } catch {
    console.warn("Failed to write JSON to clipboard.");
  }
}

export async function readJsonFromClipboard(): Promise<unknown> {
  try {
    const raw = await navigator.clipboard.readText();
    return JSON.parse(raw);
  } catch {
    console.warn("Failed to read JSON from clipboard.");
    return null;
  }
}

export function exportJson(filename: string, value: unknown) {
  downloadBlob(
    filename,
    new Blob([JSON.stringify(value, null, 2)], {
      type: "application/json",
    }),
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
