/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { Select as BaseSelect } from "@base-ui/react/select";
import { IconChevronDown } from "@tabler/icons-react";
import { cva, type VariantProps } from "class-variance-authority";
import {
  type ComponentProps,
  createContext,
  type ReactNode,
  useContext,
  useMemo,
} from "react";

import { cn } from "~/renderer/common/components/utils";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
 * A selectable option. Options are the single source for both the popup items
 * and the trigger label.
 */
export interface SelectOption<Value extends string = string> {
  value: Value;
  label: ReactNode;
  icon?: ReactNode;
}

const SelectOptionsContext = createContext<readonly SelectOption[] | null>(
  null,
);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

interface SelectRootProps<Value extends string = string> extends Omit<
  ComponentProps<typeof BaseSelect.Root>,
  "items" | "onValueChange"
> {
  value?: Value;
  defaultValue?: Value;
  options: readonly SelectOption<Value>[];
  onValueChange?: (value: Value) => void;
}

function SelectRoot<Value extends string = string>({
  value,
  defaultValue,
  options,
  onValueChange,
  children,
  ...props
}: Readonly<SelectRootProps<Value>>) {
  // Base UI renders `items` labels inside `Select.Value`; include the icon so
  // the trigger shows the full option.
  const items = useMemo(
    () =>
      options.map(({ value, label, icon }) => ({
        value,
        label: (
          <span className="flex min-w-0 items-center gap-2 truncate [&_svg]:size-[1.5em] [&_svg]:shrink-0 [&_svg]:text-(--neutral-6)">
            {icon}
            {label}
          </span>
        ),
      })),
    [options],
  );

  return (
    <SelectOptionsContext.Provider value={options}>
      <BaseSelect.Root
        {...props}
        items={items}
        value={value}
        defaultValue={defaultValue}
        onValueChange={(value) => {
          onValueChange?.(value as Value);
        }}
      >
        {children}
      </BaseSelect.Root>
    </SelectOptionsContext.Provider>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

const selectTriggerVariants = cva(
  "inline-flex h-6 cursor-pointer items-center gap-2 border border-(--neutral-4) bg-(--neutral-1) px-2 text-(length:--text-1) leading-(--leading-1) text-(--neutral-8) transition-colors select-none hover:border-(--neutral-5) hover:bg-(--neutral-2) focus-visible:outline-2 focus-visible:outline-offset-1 focus-visible:outline-(--accent-6) disabled:cursor-not-allowed disabled:opacity-40 [&_svg]:size-[1.5em] [&_svg]:shrink-0",
  {
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
  },
);

interface SelectTriggerProps
  extends
    ComponentProps<typeof BaseSelect.Trigger>,
    VariantProps<typeof selectTriggerVariants> {}

function SelectTrigger({
  radius,
  className,
  children,
  ...props
}: Readonly<SelectTriggerProps>) {
  return (
    <BaseSelect.Trigger
      {...props}
      className={cn(selectTriggerVariants({ radius }), className)}
    >
      <span className="flex min-w-0 flex-1 items-center gap-2 truncate">
        {children ?? <BaseSelect.Value />}
      </span>
      <BaseSelect.Icon>
        <IconChevronDown className="text-(--neutral-6) opacity-60" />
      </BaseSelect.Icon>
    </BaseSelect.Trigger>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

function SelectContent({
  className,
  children,
  ...props
}: Readonly<ComponentProps<typeof BaseSelect.Popup>>) {
  const options = useContext(SelectOptionsContext);

  children ??= options?.map(({ value, label, icon }) => (
    <SelectItem key={value} value={value}>
      {icon}
      {label}
    </SelectItem>
  ));

  return (
    <BaseSelect.Portal>
      <BaseSelect.Positioner sideOffset={4}>
        <BaseSelect.ScrollUpArrow />
        <BaseSelect.Popup
          {...props}
          className={cn(
            "z-50 max-h-60 min-w-32 overflow-auto rounded border border-(--neutral-4) bg-(--neutral-1) py-1 shadow-(--shadow-popup)",
            className,
          )}
        >
          {children}
        </BaseSelect.Popup>
        <BaseSelect.ScrollDownArrow />
      </BaseSelect.Positioner>
    </BaseSelect.Portal>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

function SelectItem({
  className,
  children,
  ...props
}: Readonly<ComponentProps<typeof BaseSelect.Item>>) {
  return (
    <BaseSelect.Item
      {...props}
      className={cn(
        "flex cursor-pointer items-center gap-2 px-3 py-1 text-(length:--text-1) leading-(--leading-1) text-(--neutral-8) transition-colors select-none data-highlighted:bg-(--accent-1) data-highlighted:text-(--accent-8) data-selected:font-medium data-selected:text-(--accent-8) [&_svg]:size-[1.5em] [&_svg]:shrink-0",
        className,
      )}
    >
      {children}
    </BaseSelect.Item>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export const Select = Object.assign(SelectRoot, {
  Root: SelectRoot,
  Trigger: SelectTrigger,
  Content: SelectContent,
  Item: SelectItem,
});

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
