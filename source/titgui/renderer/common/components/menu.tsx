/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { IconX } from "@tabler/icons-react";
import { cva } from "class-variance-authority";
import {
  Activity,
  createContext,
  Fragment,
  type ReactElement,
  type ReactNode,
  useContext,
  useEffect,
  useMemo,
  useState,
} from "react";
import { z } from "zod";

import { IconButton } from "~/renderer/common/components/button";
import { chrome, surface } from "~/renderer/common/components/classes";
import { ErrorGuard } from "~/renderer/common/components/error-guard";
import { Resizable } from "~/renderer/common/components/resizable";
import { ScrollArea } from "~/renderer/common/components/scroll-area";
import { Separator } from "~/renderer/common/components/separator";
import { Text } from "~/renderer/common/components/text";
import { Tooltip } from "~/renderer/common/components/tooltip";
import { cn } from "~/renderer/common/components/utils";
import { useWindowState } from "~/renderer/common/hooks/use-window";
import { assert, iota } from "~/shared/utils";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
 * A single menu entry. The `id` is a stable identifier used to persist the
 * active item across sessions — never derive it from the item position.
 */
export interface MenuItem {
  id: string;
  name: string;
  icon: ReactElement;
  group?: number;
  content?: ReactNode;
}

interface MenuRootProps {
  side: "left" | "right" | "top" | "bottom";
  items: readonly MenuItem[];
}

const menuTriggerVariants = cva(
  "flex cursor-pointer items-center justify-center transition-colors select-none",
  {
    variants: {
      orientation: {
        vertical: "size-14 border-l-[3px] [&_svg]:size-8",
        horizontal: "h-8 gap-1 border-b-2 [&_svg]:size-4",
      },
      state: {
        active: "text-(--neutral-10)",
        inactive: "text-(--neutral-6) hover:text-(--neutral-10)",
      },
    },
    compoundVariants: [
      {
        orientation: "vertical",
        state: "active",
        className: "border-l-(--accent-6)",
      },
      {
        orientation: "vertical",
        state: "inactive",
        className: "border-l-transparent",
      },
      {
        orientation: "horizontal",
        state: "active",
        className: "border-b-(--accent-6)",
      },
      {
        orientation: "horizontal",
        state: "inactive",
        className: "border-b-transparent",
      },
    ],
    defaultVariants: {
      orientation: "vertical",
      state: "inactive",
    },
  },
);

function MenuRoot({ side, items }: Readonly<MenuRootProps>) {
  // ---- State. --------------------------------------------------------------

  const [size, setSize] = useWindowState(
    `menu.${side}.size`,
    z.int().min(160).max(640),
    320,
  );

  const [activeItemID, setActiveItemID] = useWindowState(
    `menu.${side}.active-item`,
    z.string().nullable(),
    null,
  );

  function toggleItem(id: string) {
    setActiveItemID((prev) => (id === prev ? null : id));
  }

  function closeItem() {
    setActiveItemID(null);
  }

  const activeItem = items.find((item) => item.id === activeItemID);

  // ---- Layout. --------------------------------------------------------------

  const vertical = side === "left" || side === "right";
  const orientation = vertical ? "vertical" : "horizontal";
  const oppositeSide = (() => {
    switch (side) {
      case "left":
        return "right";
      case "right":
        return "left";
      case "top":
        return "bottom";
      case "bottom":
        return "top";
    }
  })();

  const directionClass = (() => {
    switch (side) {
      case "left":
        return "flex-row";
      case "right":
        return "flex-row-reverse";
      case "top":
        return "flex-col";
      case "bottom":
        return "flex-col-reverse";
    }
  })();

  const maxGroup = items.reduce((max, item) => {
    return Math.max(max, item.group ?? 0);
  }, 0);

  return (
    <div className={cn("flex", directionClass)}>
      {/* ---- Menu bar. --------------------------------------------------- */}
      <div
        className={cn(
          "flex items-center justify-between",
          vertical ? "h-full flex-col" : "w-full flex-row px-4",
          chrome(),
        )}
      >
        {iota(maxGroup + 1).map((group) => (
          <div
            key={group}
            className={cn(
              "flex items-center",
              vertical ? "flex-col" : "flex-row gap-4",
            )}
          >
            {items.map(
              (item, index) =>
                (item.group ?? 0) === group && (
                  <Fragment key={item.id}>
                    {vertical || index === 0 || (
                      <Separator orientation="vertical" />
                    )}
                    <Tooltip content={item.name} side={oppositeSide}>
                      <button
                        type="button"
                        onClick={() => {
                          toggleItem(item.id);
                        }}
                        aria-label={item.name}
                        aria-pressed={item.id === activeItemID}
                        className={menuTriggerVariants({
                          orientation,
                          state:
                            item.id === activeItemID ? "active" : "inactive",
                        })}
                      >
                        {item.icon}
                        {vertical || (
                          <span className="text-(length:--text-1) leading-(--leading-1)">
                            {item.name}
                          </span>
                        )}
                      </button>
                    </Tooltip>
                  </Fragment>
                ),
            )}
          </div>
        ))}
      </div>

      {/* ---- Menu pane. -------------------------------------------------- */}
      <Activity mode={activeItem === undefined ? "hidden" : "visible"}>
        <Resizable
          side={side}
          size={size}
          setSize={setSize}
          minSize={160}
          maxSize={640}
        >
          {items.map((item) => (
            <Activity
              key={item.id}
              mode={item.id === activeItemID ? "visible" : "hidden"}
            >
              <MenuPane name={item.name} onClose={closeItem}>
                {item.content}
              </MenuPane>
            </Activity>
          ))}
        </Resizable>
      </Activity>
    </div>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export interface MenuAction {
  name: string;
  disabled?: boolean;
  icon: ReactElement;
  onClick: () => void;
}

export interface MenuActions {
  addAction: (action: MenuAction) => () => void;
}

const MenuActionsContext = createContext<MenuActions | null>(null);

function useMenuActions(): MenuActions {
  const context = useContext(MenuActionsContext);
  assert(context !== null, "Menu actions are not available.");
  return context;
}

export function useMenuAction(action: MenuAction) {
  const { addAction } = useMenuActions();
  useEffect(() => addAction(action), [action, addAction]);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

interface MenuPaneProps {
  name: string;
  onClose: () => void;
  children?: ReactNode;
}

function MenuPane({ name, onClose, children }: Readonly<MenuPaneProps>) {
  // ---- Actions. -------------------------------------------------------------

  const [actions, setActions] = useState<MenuAction[]>([]);

  const menuActions = useMemo<MenuActions>(
    () => ({
      addAction(action) {
        const filterByName = ({ name }: MenuAction) => name !== action.name;
        setActions((prev) => [
          ...prev.filter((action) => filterByName(action)),
          action,
        ]);
        return () => {
          setActions((prev) => prev.filter((action) => filterByName(action)));
        };
      },
    }),
    [],
  );

  // ---- Layout. --------------------------------------------------------------

  return (
    <div className={cn("flex h-full flex-col", chrome())}>
      {/* ---- Header. ----------------------------------------------------- */}
      <div className="flex h-8 shrink-0 items-center gap-2 px-1">
        <div className="grow">
          <Text variant="label" size="2" color="muted" truncate>
            {name}
          </Text>
        </div>

        {actions.map((action) => (
          <Tooltip key={action.name} content={action.name}>
            <IconButton
              disabled={action.disabled}
              onClick={action.onClick}
              aria-label={action.name}
            >
              {action.icon}
            </IconButton>
          </Tooltip>
        ))}

        <Tooltip content="Close">
          <IconButton onClick={onClose} aria-label="Close">
            <IconX />
          </IconButton>
        </Tooltip>
      </div>

      {/* ---- Contents. --------------------------------------------------- */}
      <div className={cn("mx-1 mb-1 grow overflow-auto", surface())}>
        <ErrorGuard>
          <ScrollArea>
            <MenuActionsContext.Provider value={menuActions}>
              {children}
            </MenuActionsContext.Provider>
          </ScrollArea>
        </ErrorGuard>
      </div>
    </div>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export const Menu = Object.assign(MenuRoot, {
  Root: MenuRoot,
});

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
