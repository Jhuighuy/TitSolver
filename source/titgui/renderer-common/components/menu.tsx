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

import { IconButton } from "~/renderer-common/components/button";
import { chrome, surface } from "~/renderer-common/components/classes";
import { Box, Flex } from "~/renderer-common/components/layout";
import { Resizable } from "~/renderer-common/components/resizable";
import { ScrollArea } from "~/renderer-common/components/scroll-area";
import { Separator } from "~/renderer-common/components/separator";
import { Text } from "~/renderer-common/components/text";
import { Tooltip } from "~/renderer-common/components/tooltip";
import { cn } from "~/renderer-common/components/utils";
import { useWindowState } from "~/renderer-common/hooks/use-window";
import { assert, iota, items } from "~/shared/utils";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

const menuTriggerVariants = cva(
  "cursor-pointer transition-colors select-none",
  {
    variants: {
      orientation: {
        vertical: "border-l-[3px] [&_svg]:size-8",
        horizontal: "border-b-2 [&_svg]:size-4",
      },
      state: {
        active: "text-(--fg-1)",
        inactive: "text-(--fg-4) hover:text-(--fg-1)",
      },
    },
    compoundVariants: [
      {
        orientation: "vertical",
        state: "active",
        className: "border-l-(--accent-fg-3)",
      },
      {
        orientation: "vertical",
        state: "inactive",
        className: "border-l-transparent",
      },
      {
        orientation: "horizontal",
        state: "active",
        className: "border-b-(--accent-fg-3)",
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

interface MenuRootProps {
  side: "left" | "right" | "top" | "bottom";
  children: ReactElement<MenuItemProps> | ReactElement<MenuItemProps>[];
}

function MenuRoot({ side, children }: Readonly<MenuRootProps>) {
  // ---- State. --------------------------------------------------------------

  const [size, setSize] = useWindowState(
    `menu.${side}.size`,
    z.int().min(160).max(640),
    320,
  );

  const [activeItem, setActiveItem] = useWindowState(
    `menu.${side}.active-item`,
    z.int().min(-1).max(items(children).length - 1), // prettier-ignore
    -1,
  );

  function setActiveItemOrToggle(index: number) {
    setActiveItem((prev) => (index === prev ? -1 : index));
  }

  // ---- Layout. --------------------------------------------------------------

  const vertical = side === "left" || side === "right";
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

  const direction = (() => {
    switch (side) {
      case "left":
        return "row";
      case "right":
        return "row-reverse";
      case "top":
        return "column";
      case "bottom":
        return "column-reverse";
    }
  })();

  const childrenArray = items(children);
  const maxGroup = childrenArray.reduce(
    (max, c) => Math.max(max, c.props.group),
    0,
  );

  return (
    <Flex direction={direction} gap="1px">
      {/* ---- Menu bar. --------------------------------------------------- */}
      <Flex
        align="center"
        justify="between"
        {...(vertical
          ? { direction: "column", height: "100%" }
          : { direction: "row", width: "100%", px: "4" })}
        className={chrome()}
      >
        {iota(maxGroup + 1).map((group) => (
          <Flex
            key={group}
            align="center"
            {...(vertical
              ? { direction: "column" }
              : { direction: "row", gap: "4" })}
          >
            {childrenArray.map(
              (item, index) =>
                item.props.group === group &&
                (vertical ? (
                  <Tooltip
                    key={[group, item.props.name].join("-")}
                    content={item.props.name}
                    side={oppositeSide}
                  >
                    <Flex
                      align="center"
                      justify="center"
                      size="14"
                      onClick={() => {
                        setActiveItemOrToggle(index);
                      }}
                      aria-label={item.props.name}
                      className={menuTriggerVariants({
                        orientation: "vertical",
                        state: index === activeItem ? "active" : "inactive",
                      })}
                    >
                      {item.props.icon}
                    </Flex>
                  </Tooltip>
                ) : (
                  <Fragment key={item.props.name}>
                    {index > 0 && <Separator orientation="vertical" />}
                    <Flex
                      align="center"
                      justify="center"
                      gap="1"
                      height="8"
                      onClick={() => {
                        setActiveItemOrToggle(index);
                      }}
                      aria-label={item.props.name}
                      className={menuTriggerVariants({
                        orientation: "horizontal",
                        state: index === activeItem ? "active" : "inactive",
                      })}
                    >
                      {item.props.icon}
                      <span className="text-(length:--text-1) leading-(--leading-1)">
                        {item.props.name}
                      </span>
                    </Flex>
                  </Fragment>
                )),
            )}
          </Flex>
        ))}
      </Flex>

      {/* ---- Menu item. -------------------------------------------------- */}
      <Activity mode={activeItem === -1 ? "hidden" : "visible"}>
        <Resizable
          side={side}
          size={size}
          setSize={setSize}
          minSize={160}
          maxSize={640}
        >
          {childrenArray.map((item, index) => (
            <Activity
              key={item.props.name}
              mode={index === activeItem ? "visible" : "hidden"}
            >
              {item}
            </Activity>
          ))}
        </Resizable>
      </Activity>
    </Flex>
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

interface MenuItemProps {
  group: number;
  name: string;
  icon: ReactElement;
  children?: ReactNode;
}

function MenuItem({ name, children }: Readonly<MenuItemProps>) {
  // ---- Actions. -------------------------------------------------------------

  const [actions, setActions] = useState<MenuAction[]>([]);

  const menuActions = useMemo<MenuActions>(
    () => ({
      addAction(action) {
        const filterByName = ({ name }: MenuAction) => name !== action.name;
        setActions((prev) => [...prev.filter(filterByName), action]);
        return () => {
          setActions((prev) => prev.filter(filterByName));
        };
      },
    }),
    [],
  );

  // ---- Layout. --------------------------------------------------------------

  return (
    <Flex direction="column" height="100%" px="1" pb="1" className={chrome()}>
      {/* ---- Header. ----------------------------------------------------- */}
      <Flex gap="2" align="center" height="8" minHeight="8" maxHeight="8">
        <Box flexGrow="1">
          <Text variant="label" size="2" color="muted" truncate>
            {name}
          </Text>
        </Box>

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
          <IconButton
            onClick={undefined /** @todo Implement me. */}
            aria-label="Close"
          >
            <IconX />
          </IconButton>
        </Tooltip>
      </Flex>

      {/* ---- Contents. --------------------------------------------------- */}
      <Box flexGrow="1" overflow="auto" className={cn("rounded-lg", surface())}>
        <ScrollArea>
          <MenuActionsContext.Provider value={menuActions}>
            {children}
          </MenuActionsContext.Provider>
        </ScrollArea>
      </Box>
    </Flex>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export const Menu = Object.assign(MenuRoot, {
  Root: MenuRoot,
  Item: MenuItem,
});

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
