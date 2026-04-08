/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { cva, type VariantProps } from "class-variance-authority";
import type { ComponentProps } from "react";

import { surface } from "~/renderer-common/components/classes";
import { cn } from "~/renderer-common/components/utils";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

const cardVariants = cva("p-2", {
  variants: {
    radius: {
      none: "rounded-none",
      small: "rounded-sm",
      medium: "rounded",
      large: "rounded-lg",
      full: "rounded-full",
    },
  },
  defaultVariants: {
    radius: "medium",
  },
});

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

interface CardProps
  extends ComponentProps<"div">, VariantProps<typeof cardVariants> {}

export function Card({ radius, className, ...props }: Readonly<CardProps>) {
  return (
    <div
      {...props}
      className={cn(cardVariants({ radius }), surface(), className)}
    />
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
