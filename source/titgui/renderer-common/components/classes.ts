/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { cva } from "class-variance-authority";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export const chrome = cva(
  "border border-slate-300/90 from-slate-200 to-slate-300 shadow-[inset_0_1px_0_rgba(255,255,255,0.45)] dark:border-slate-700/60 dark:from-slate-700 dark:to-slate-800 dark:shadow-[inset_0_1px_0_rgba(255,255,255,0.08)]",
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

export const surface = cva(
  "border border-slate-300/90 bg-slate-100 dark:border-slate-700/60 dark:bg-slate-900",
);

export const hoverSurface = cva(
  "hover:bg-slate-300/70 dark:hover:bg-slate-600/60",
);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
