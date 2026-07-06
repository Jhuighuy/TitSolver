/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { Tabs as BaseTabs } from "@base-ui/react/tabs";
import { IconPlus, IconX } from "@tabler/icons-react";
import { createContext, useContext, useMemo, type ComponentProps } from "react";

import { IconButton } from "~/renderer/common/components/button";
import { chrome, hoverSurface } from "~/renderer/common/components/classes";
import { Text } from "~/renderer/common/components/text";
import { cn } from "~/renderer/common/components/utils";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
 * Tab identifier type. Kept internally as `unknown` in the context; the
 * public components are generic over it.
 */
export type TabValue = string | number;

interface TabsContextValue {
  onAddTab?: () => void;
  onCloseTab?: (value: TabValue) => void;
}

const TabsContext = createContext<TabsContextValue | null>(null);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

interface TabsRootProps<Value extends TabValue> extends Omit<
  ComponentProps<typeof BaseTabs.Root>,
  "value" | "defaultValue" | "onValueChange"
> {
  value?: Value | null;
  defaultValue?: Value;
  onValueChange?: (value: Value) => void;
  onAddTab?: () => void;
  onCloseTab?: (value: Value) => void;
}

function TabsRoot<Value extends TabValue>({
  value,
  defaultValue,
  onValueChange,
  onAddTab,
  onCloseTab,
  className,
  ...props
}: Readonly<TabsRootProps<Value>>) {
  const context = useMemo<TabsContextValue>(
    () => ({
      onAddTab,
      onCloseTab: onCloseTab as ((value: TabValue) => void) | undefined,
    }),
    [onAddTab, onCloseTab],
  );

  return (
    <TabsContext.Provider value={context}>
      <BaseTabs.Root
        {...props}
        value={value}
        defaultValue={defaultValue}
        onValueChange={(value) => {
          onValueChange?.(value as Value);
        }}
        className={cn("flex h-full w-full flex-col", className)}
      />
    </TabsContext.Provider>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

function TabsList({
  children,
  className,
  ...props
}: Readonly<ComponentProps<typeof BaseTabs.List>>) {
  const context = useContext(TabsContext);

  return (
    <BaseTabs.List
      {...props}
      className={cn(
        "flex h-8 w-full min-w-0 items-end px-1 pt-1",
        chrome(),
        className,
      )}
    >
      <div className="flex min-w-0 items-center justify-end gap-0.5">
        {children}
        {context?.onAddTab !== undefined && (
          <IconButton
            aria-label="Add Tab"
            onClick={() => {
              context.onAddTab?.();
            }}
          >
            <IconPlus />
          </IconButton>
        )}
      </div>
    </BaseTabs.List>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

interface TabsTabProps<Value extends TabValue> extends Omit<
  ComponentProps<typeof BaseTabs.Tab>,
  "value"
> {
  value: Value;
}

function TabsTab<Value extends TabValue>({
  value,
  children,
  onMouseDown,
  className,
  ...props
}: Readonly<TabsTabProps<Value>>) {
  const context = useContext(TabsContext);

  return (
    <BaseTabs.Tab
      value={value}
      {...props}
      onMouseDown={(event) => {
        onMouseDown?.(event);
        if (event.defaultPrevented || event.button !== 1) return;
        event.preventDefault();
        context?.onCloseTab?.(value);
      }}
      className={cn(
        "flex h-7 w-50 min-w-0 flex-1 cursor-pointer items-center justify-between gap-1 rounded-t-lg border border-b-0 border-(--neutral-4) bg-(--neutral-2) px-2 py-2 transition-colors data-active:border-(--neutral-5) data-active:bg-(--neutral-1) data-disabled:cursor-not-allowed data-disabled:opacity-40",
        hoverSurface(),
        className,
      )}
    >
      <Text color="muted" truncate>
        {children}
      </Text>
      {context?.onCloseTab !== undefined && (
        <IconButton
          aria-label="Close Tab"
          onClick={(event) => {
            event.stopPropagation();
            context.onCloseTab?.(value);
          }}
        >
          <IconX />
        </IconButton>
      )}
    </BaseTabs.Tab>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

function TabsPanel({
  className,
  ...props
}: Readonly<ComponentProps<typeof BaseTabs.Panel>>) {
  return (
    <BaseTabs.Panel {...props} className={cn("min-h-0 flex-1", className)} />
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export const Tabs = Object.assign(TabsRoot, {
  Root: TabsRoot,
  List: TabsList,
  Tab: TabsTab,
  Panel: TabsPanel,
});

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
