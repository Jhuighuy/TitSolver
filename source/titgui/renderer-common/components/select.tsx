/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { Select as BaseSelect } from "@base-ui/react/select";
import { IconChevronDown } from "@tabler/icons-react";
import { cva, type VariantProps } from "class-variance-authority";
import {
  Children,
  type ComponentProps,
  createContext,
  isValidElement,
  type ReactNode,
  useContext,
  useMemo,
  useState,
} from "react";

import { cn } from "~/renderer-common/components/utils";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

interface SelectContext {
  currentValue: string | null;
  registry: ReadonlyMap<string, ReactNode>;
}

const SelectContext = createContext<SelectContext | null>(null);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

interface SelectRootProps<Value = string> extends Omit<
  ComponentProps<typeof BaseSelect.Root>,
  "onValueChange"
> {
  value?: Value;
  defaultValue?: Value;
  onValueChange?: (value: Value) => void;
}

function SelectRoot<Value = string>({
  value,
  defaultValue,
  onValueChange,
  children,
  ...props
}: Readonly<SelectRootProps<Value>>) {
  const [internalValue, setInternalValue] = useState(
    (defaultValue as string | undefined) ?? null,
  );

  const currentValue = value === undefined ? internalValue : (value as string);

  const registry = useMemo<ReadonlyMap<string, ReactNode>>(() => {
    const map = new Map<string, ReactNode>();

    function collectItems(node: ReactNode) {
      Children.forEach(node, (child) => {
        if (!isValidElement(child)) return;

        const props = child.props as Record<string, unknown>;
        if (child.type === SelectItem) {
          const val = props.value;
          if (typeof val === "string" || typeof val === "number") {
            map.set(String(val), props.children as ReactNode);
          }
          return; // Don't recurse into item children.
        }

        if (props.children != null) collectItems(props.children as ReactNode);
      });
    }

    collectItems(children);

    return map;
  }, [children]);

  const context = useMemo<SelectContext>(
    () => ({ currentValue, registry }),
    [currentValue, registry],
  );

  return (
    <SelectContext.Provider value={context}>
      <BaseSelect.Root
        {...(props as ComponentProps<typeof BaseSelect.Root>)}
        value={value}
        defaultValue={defaultValue}
        onValueChange={(value) => {
          setInternalValue(value as string);
          onValueChange?.(value as Value);
        }}
      >
        {children}
      </BaseSelect.Root>
    </SelectContext.Provider>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

const selectTriggerVariants = cva(
  "inline-flex h-6 cursor-pointer items-center gap-2 border border-(--chrome-1) bg-(--bg-3) px-2 text-(length:--text-1) leading-(--leading-1) text-(--fg-2) transition-colors select-none hover:border-(--chrome-2) hover:bg-(--bg-4) focus-visible:outline-2 focus-visible:outline-offset-1 focus-visible:outline-(--accent-fg-3) disabled:cursor-not-allowed disabled:opacity-40 [&_svg]:size-[1.5em] [&_svg]:shrink-0",
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
  const context = useContext(SelectContext);

  children ??=
    context === null
      ? null
      : (context.registry.get(context.currentValue ?? "") ?? null);

  return (
    <BaseSelect.Trigger
      {...props}
      className={cn(selectTriggerVariants({ radius }), className)}
    >
      <span className="flex min-w-0 flex-1 items-center gap-2 truncate [&_svg]:text-(--fg-4)">
        {children}
      </span>
      <BaseSelect.Icon>
        <IconChevronDown className="text-(--fg-4) opacity-60" />
      </BaseSelect.Icon>
    </BaseSelect.Trigger>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

interface SelectValueProps extends ComponentProps<typeof BaseSelect.Value> {
  placeholder?: string;
}

function SelectValue({
  className,
  children,
  placeholder,
  ...props
}: Readonly<SelectValueProps>) {
  return (
    <BaseSelect.Value
      {...props}
      placeholder={placeholder}
      className={cn("flex-1 truncate", className)}
    >
      {children}
    </BaseSelect.Value>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

function SelectContent({
  className,
  children,
  ...props
}: Readonly<ComponentProps<typeof BaseSelect.Popup>>) {
  return (
    <BaseSelect.Portal>
      <BaseSelect.Positioner sideOffset={4}>
        <BaseSelect.ScrollUpArrow />
        <BaseSelect.Popup
          {...props}
          className={cn(
            "z-50 max-h-60 min-w-32 overflow-auto rounded border border-(--chrome-1) bg-(--bg-3) py-1 shadow-(--shadow-popup)",
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
  value,
  className,
  children,
  ...props
}: Readonly<ComponentProps<typeof BaseSelect.Item>>) {
  return (
    <BaseSelect.Item
      value={value as string}
      {...props}
      className={cn(
        "flex cursor-pointer items-center gap-2 px-3 py-1 text-(length:--text-1) leading-(--leading-1) text-(--fg-2) transition-colors select-none hover:bg-(--accent-bg-1) hover:text-(--accent-fg-1) data-highlighted:bg-(--accent-bg-1) data-highlighted:text-(--accent-fg-1) data-selected:font-medium data-selected:text-(--accent-fg-1) [&_svg]:size-[1.5em] [&_svg]:shrink-0",
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
  Value: SelectValue,
  Content: SelectContent,
  Item: SelectItem,
});

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
