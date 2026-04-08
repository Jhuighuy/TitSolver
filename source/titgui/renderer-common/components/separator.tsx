/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { Separator as BaseSeparator } from "@base-ui/react/separator";
import { cva, type VariantProps } from "class-variance-authority";
import type { ComponentProps } from "react";

import { cn } from "~/renderer-common/components/utils";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

const separatorVariants = cva("shrink-0 bg-(--bg-6)", {
  variants: {
    orientation: {
      horizontal: "h-px w-full",
      vertical: "w-px",
    },
    size: {
      "1": "",
      "2": "",
      full: "",
    },
  },
  compoundVariants: [
    { orientation: "vertical", size: "1", className: "h-4" },
    { orientation: "vertical", size: "2", className: "h-6" },
    { orientation: "vertical", size: "full", className: "h-full" },
  ],
  defaultVariants: {
    orientation: "horizontal",
    size: "1",
  },
});

interface SeparatorProps
  extends
    Omit<ComponentProps<typeof BaseSeparator>, "orientation">,
    VariantProps<typeof separatorVariants> {
  orientation?: "horizontal" | "vertical";
}

export function Separator({
  orientation = "horizontal",
  size,
  className,
  ...props
}: Readonly<SeparatorProps>) {
  return (
    <BaseSeparator
      {...props}
      orientation={orientation}
      className={cn(separatorVariants({ orientation, size }), className)}
    />
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
