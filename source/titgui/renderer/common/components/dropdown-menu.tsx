/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { Menu as BaseMenu } from "@base-ui/react/menu";
import { IconChevronDown } from "@tabler/icons-react";
import { cva, type VariantProps } from "class-variance-authority";
import type { ComponentProps } from "react";

import { cn } from "~/renderer/common/components/utils";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

function DropdownMenuRoot(
  props: Readonly<ComponentProps<typeof BaseMenu.Root>>,
) {
  return <BaseMenu.Root {...props} />;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

const dropdownMenuTriggerVariants = cva(
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
      variant: "ghost",
      size: "1",
      radius: "medium",
    },
  },
);

interface DropdownMenuTriggerProps
  extends
    Omit<ComponentProps<typeof BaseMenu.Trigger>, "render">,
    VariantProps<typeof dropdownMenuTriggerVariants> {}

function DropdownMenuTrigger({
  variant,
  size,
  radius,
  className,
  children,
  ...props
}: Readonly<DropdownMenuTriggerProps>) {
  return (
    <BaseMenu.Trigger
      {...props}
      className={cn(
        dropdownMenuTriggerVariants({ variant, size, radius }),
        className,
      )}
    >
      {children}
      <span className="inline-flex items-center">
        <IconChevronDown />
      </span>
    </BaseMenu.Trigger>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

function DropdownMenuContent({
  className,
  children,
  ...props
}: Readonly<ComponentProps<typeof BaseMenu.Popup>>) {
  return (
    <BaseMenu.Portal>
      <BaseMenu.Positioner sideOffset={4}>
        <BaseMenu.Popup
          {...props}
          className={cn(
            "z-50 flex min-w-40 flex-col gap-2 rounded border border-(--neutral-4) bg-(--neutral-1) p-2 shadow-(--shadow-popup)",
            className,
          )}
        >
          {children}
        </BaseMenu.Popup>
      </BaseMenu.Positioner>
    </BaseMenu.Portal>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

function DropdownMenuItem({
  className,
  children,
  ...props
}: Readonly<ComponentProps<typeof BaseMenu.Item>>) {
  return (
    <BaseMenu.Item
      {...props}
      className={cn(
        "flex cursor-pointer items-center gap-2 px-3 py-1.5 text-(length:--text-1) leading-(--leading-1) text-(--neutral-8) transition-colors select-none hover:bg-(--accent-1) hover:text-(--accent-8) data-highlighted:bg-(--accent-1) data-highlighted:text-(--accent-8) [&_svg]:size-[1.5em] [&_svg]:shrink-0",
        className,
      )}
    >
      {children}
    </BaseMenu.Item>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export const DropdownMenu = Object.assign(DropdownMenuRoot, {
  Root: DropdownMenuRoot,
  Trigger: DropdownMenuTrigger,
  Content: DropdownMenuContent,
  Item: DropdownMenuItem,
});

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
