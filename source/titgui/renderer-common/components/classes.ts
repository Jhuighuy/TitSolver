/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { cva } from "class-variance-authority";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export const chrome = cva(
  [
    "border",
    [
      "light:border-slate-300/90",
      "dark:border-slate-700/60", //
    ],
    [
      "light:shadow-[inset_0_1px_0_rgba(255,255,255,0.45)]",
      "dark:shadow-[inset_0_1px_0_rgba(255,255,255,0.08)]",
    ],
    [
      "light:from-slate-200 light:to-slate-300",
      "dark:from-slate-700 dark:to-slate-800",
    ],
  ],
  {
    variants: {
      direction: {
        br: "bg-linear-to-br",
        bl: "bg-linear-to-bl",
      },
    },
    defaultVariants: {
      direction: "br",
    },
  },
);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export const surface = cva([
  "border",
  ["light:border-slate-300/90", "dark:border-slate-700/60"],
  ["light:bg-slate-100", "dark:bg-slate-900"],
]);

export const hoverSurface = cva([
  ["light:hover:bg-slate-300/70", "dark:hover:bg-slate-600/60"],
]);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
