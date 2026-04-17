/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { Separator as BaseSeparator } from "@base-ui/react/separator";
import { cva, type VariantProps } from "class-variance-authority";
import type { ComponentProps } from "react";

import { cn } from "~/renderer-common/components/utils";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

const separatorVariants = cva("shrink-0 bg-(--fg-5)", {
  variants: {
    orientation: {
      horizontal: "h-px",
      vertical: "w-px",
    },
    size: {
      "1": "",
      "2": "",
      "3": "",
      "4": "",
      full: "",
    },
  },
  compoundVariants: [
    { orientation: "horizontal", size: "1", className: "w-4" },
    { orientation: "horizontal", size: "2", className: "w-6" },
    { orientation: "horizontal", size: "3", className: "w-8" },
    { orientation: "horizontal", size: "4", className: "w-10" },
    { orientation: "horizontal", size: "full", className: "w-full" },
    { orientation: "vertical", size: "1", className: "h-4" },
    { orientation: "vertical", size: "2", className: "h-6" },
    { orientation: "vertical", size: "3", className: "h-8" },
    { orientation: "vertical", size: "4", className: "h-10" },
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
