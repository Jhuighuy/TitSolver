/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { Tabs as BaseTabs } from "@base-ui/react/tabs";
import { IconPlus, IconX } from "@tabler/icons-react";
import { createContext, useContext, type ComponentProps } from "react";

import { IconButton } from "~/renderer-common/components/button";
import { chrome, hoverSurface } from "~/renderer-common/components/classes";
import { Flex } from "~/renderer-common/components/layout";
import { Text } from "~/renderer-common/components/text";
import { cn } from "~/renderer-common/components/utils";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

interface TabsContextValue {
  onAddTab?: () => void;
  onCloseTab?: (value: TabsValue) => void;
}

const TabsContext = createContext<TabsContextValue | null>(null);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export interface TabsProps extends ComponentProps<typeof BaseTabs.Root> {
  onAddTab?: () => void;
  onCloseTab?: (value: TabsValue) => void;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

type TabsValue = unknown;

export interface TabsTabProps extends Omit<
  ComponentProps<typeof BaseTabs.Tab>,
  "value"
> {
  value: TabsValue;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

function TabsRoot({
  onAddTab,
  onCloseTab,
  className,
  ...props
}: Readonly<TabsProps>) {
  return (
    <TabsContext.Provider value={{ onAddTab, onCloseTab }}>
      <BaseTabs.Root
        {...props}
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
        "flex h-8 items-end px-1 pt-1",
        chrome({ direction: "bl" }),
        className,
      )}
    >
      <Flex align="center" justify="end" gap="1px">
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
      </Flex>
    </BaseTabs.List>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

function TabsTab({
  value,
  children,
  onMouseDown,
  className,
  ...props
}: Readonly<TabsTabProps>) {
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
        "flex h-7 max-w-70 min-w-0 cursor-pointer items-center gap-2 rounded-t-lg border border-b-0 border-(--chrome-1) bg-(--bg-2) px-3 py-2 transition-colors data-active:border-(--chrome-2) data-active:bg-(--bg-3) data-disabled:cursor-not-allowed data-disabled:opacity-40",
        hoverSurface(),
        className,
      )}
    >
      <Text color="muted">{children}</Text>
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
