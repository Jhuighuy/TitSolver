/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { Button as BaseButton } from "@base-ui/react/button";
import { cva, type VariantProps } from "class-variance-authority";
import type { ComponentProps } from "react";

import { cn } from "~/renderer/common/components/utils";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

const buttonVariants = cva(
  "inline-flex cursor-pointer items-center justify-center gap-1.5 rounded border font-(--font-sans) transition-colors select-none focus-visible:outline-2 focus-visible:outline-offset-1 focus-visible:outline-(--accent-6) disabled:cursor-not-allowed disabled:opacity-40 [&_svg]:size-[1.5em] [&_svg]:shrink-0",
  {
    variants: {
      variant: {
        solid:
          "border-transparent bg-(--accent-6) text-(--accent-contrast) hover:bg-(--accent-7) active:bg-(--accent-7)",
        outline:
          "border-(--accent-6) text-(--neutral-8) hover:bg-(--neutral-2)",
        ghost:
          "border-transparent text-(--neutral-7) hover:bg-(--neutral-10)/10 hover:text-(--neutral-10)",
      },
      size: {
        "1": "h-6 px-2 py-0.5 text-(length:--text-1) leading-(--leading-1)",
        "2": "h-7 px-3 py-1 text-(length:--text-2) leading-(--leading-2)",
        "3": "h-8 px-4 py-1.5 text-(length:--text-3) leading-(--leading-3)",
      },
      radius: {
        none: "rounded-none",
        small: "rounded-sm",
        medium: "rounded",
        large: "rounded-lg",
        full: "rounded-full",
      },
    },
    defaultVariants: {
      variant: "solid",
      size: "1",
      radius: "medium",
    },
  },
);

interface ButtonProps
  extends
    ComponentProps<typeof BaseButton>,
    VariantProps<typeof buttonVariants> {}

export function Button({
  variant,
  size,
  radius,
  className,
  children,
  ...props
}: Readonly<ButtonProps>) {
  return (
    <BaseButton
      {...props}
      className={cn(buttonVariants({ variant, size, radius }), className)}
    >
      {children}
    </BaseButton>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

const iconButtonVariants = cva(
  "flex shrink-0 cursor-pointer items-center justify-center border-transparent text-(--neutral-7) transition-colors select-none hover:bg-(--neutral-10)/10 hover:text-(--neutral-10) focus-visible:outline-2 focus-visible:outline-offset-1 focus-visible:outline-(--accent-6) disabled:cursor-not-allowed disabled:opacity-40 [&_svg]:shrink-0",
  {
    variants: {
      active: {
        true: "bg-(--neutral-10)/10 text-(--accent-7)",
      },
      size: {
        "1": "h-5 w-5 [&_svg]:size-3",
        "2": "h-6 w-6 [&_svg]:size-4",
        "3": "h-7 w-7 [&_svg]:size-5",
      },
      radius: {
        none: "rounded-none",
        small: "rounded-sm",
        medium: "rounded",
        large: "rounded-lg",
        full: "rounded-full",
      },
    },
    defaultVariants: {
      size: "1",
      radius: "medium",
    },
  },
);

interface IconButtonProps
  extends
    ComponentProps<typeof BaseButton>,
    VariantProps<typeof iconButtonVariants> {}

export function IconButton({
  active,
  size,
  radius,
  className,
  ...props
}: Readonly<IconButtonProps>) {
  return (
    <BaseButton
      {...props}
      className={cn(iconButtonVariants({ active, size, radius }), className)}
    />
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
