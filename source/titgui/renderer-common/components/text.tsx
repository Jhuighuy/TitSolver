/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { cva, type VariantProps } from "class-variance-authority";
import type { ComponentProps, ElementType } from "react";

import { cn } from "~/renderer-common/components/utils";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

const textVariants = cva([], {
  variants: {
    variant: {
      body: "",
      label: "font-semibold uppercase",
    },
    size: {
      "1": "text-(length:--text-1) leading-(--leading-1)",
      "2": "text-(length:--text-2) leading-(--leading-2)",
      "3": "text-(length:--text-3) leading-(--leading-3)",
    },
    weight: {
      regular: "font-normal",
      medium: "font-medium",
      bold: "font-bold",
    },
    color: {
      strong: "text-(--fg-1)",
      default: "text-(--fg-2)",
      muted: "text-(--fg-3)",
      subtle: "text-(--fg-4)",
      accent: "text-(--accent-fg-2)",
      red: "text-red-600 dark:text-red-400",
    },
    truncate: {
      true: "truncate",
    },
    mono: {
      true: "font-(--font-mono)",
    },
  },
  compoundVariants: [
    {
      variant: "label",
      size: "1",
      class: "tracking-(--tracking-label-1)",
    },
    {
      variant: "label",
      size: "2",
      class: "tracking-(--tracking-label-2)",
    },
    {
      variant: "label",
      size: "3",
      class: "tracking-(--tracking-label-2)",
    },
  ],
  defaultVariants: {
    variant: "body",
    size: "1",
    weight: "regular",
    color: "default",
  },
});

export interface TextProps
  extends
    Omit<ComponentProps<"span">, "color">,
    VariantProps<typeof textVariants> {
  as?: ElementType;
}

export function Text({
  as: Component = "span",
  variant,
  size,
  weight,
  color,
  truncate,
  mono,
  className,
  ...props
}: Readonly<TextProps>) {
  return (
    <Component
      {...props}
      className={cn(
        textVariants({ variant, size, weight, color, truncate, mono }),
        className,
      )}
    />
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export function Strong({
  className,
  ...props
}: Readonly<ComponentProps<"strong">>) {
  return <strong {...props} className={cn("font-bold", className)} />;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export function Mono({
  className,
  ...props
}: Readonly<ComponentProps<"code">>) {
  return <code {...props} className={cn("font-(--font-mono)", className)} />;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
