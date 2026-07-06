/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import fs from "node:fs";
import path from "node:path";

import { describe, expect, it } from "vitest";

import { windowBackgroundColors } from "~/shared/theme";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Hex values of the Tailwind palette entries used by the application
// background token. Extend when `--neutral-3` moves to a different shade.
const paletteHex: Record<string, string> = {
  "slate-200": "#e2e8f0",
  "slate-900": "#0f172a",
};

// Extract the palette shade referenced by `--neutral-3` from a CSS block.
function extractAppBackground(cssBlock: string): string {
  const match = /--neutral-3:\s*var\(--color-([a-z]+-\d+)\)/u.exec(cssBlock);
  expect(match).not.toBeNull();
  const shade = (match as RegExpExecArray)[1];
  expect(paletteHex).toHaveProperty(shade);
  return paletteHex[shade];
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

describe("windowBackgroundColors", () => {
  const css = fs.readFileSync(
    path.join(__dirname, "..", "renderer", "index.css"),
    "utf8",
  );

  const darkBlockStart = css.indexOf("@media (prefers-color-scheme: dark)");
  expect(darkBlockStart).toBeGreaterThan(0);
  const lightBlock = css.slice(0, darkBlockStart);
  const darkBlock = css.slice(darkBlockStart);

  it("matches the light `--neutral-3` token", () => {
    expect(windowBackgroundColors.light).toBe(extractAppBackground(lightBlock));
  });

  it("matches the dark `--neutral-3` token", () => {
    expect(windowBackgroundColors.dark).toBe(extractAppBackground(darkBlock));
  });
});

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
