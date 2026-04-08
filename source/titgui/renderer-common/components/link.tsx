/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { cva, type VariantProps } from "class-variance-authority";
import type { ComponentProps } from "react";

import { cn } from "~/renderer-common/components/utils";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

const linkVariants = cva(
  "inline-flex cursor-pointer items-center gap-1 text-(--accent-fg-2) transition-colors hover:underline focus-visible:outline-2 focus-visible:outline-offset-1 focus-visible:outline-(--accent-fg-3) [&_svg]:size-[1.5em] [&_svg]:shrink-0",
  {
    variants: {
      weight: {
        regular: "font-normal",
        medium: "font-medium",
        bold: "font-bold",
      },
    },
    defaultVariants: {
      weight: "regular",
    },
  },
);

export interface LinkProps
  extends ComponentProps<"a">, VariantProps<typeof linkVariants> {}

export function Link({ weight, className, ...props }: Readonly<LinkProps>) {
  return <a {...props} className={cn(linkVariants({ weight }), className)} />;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
