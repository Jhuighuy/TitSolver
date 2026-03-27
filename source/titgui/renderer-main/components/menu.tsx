/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import {
  Box,
  Flex,
  IconButton,
  ScrollArea,
  Separator,
  Text,
  Tooltip,
} from "@radix-ui/themes";
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
import { TbMinimize as MinimizeIcon } from "react-icons/tb";
import { z } from "zod";

import { chrome, surface } from "~/renderer-common/components/classes";
import { Resizable } from "~/renderer-common/components/resizable";
import { cn } from "~/renderer-common/components/utils";
import { usePersistedState } from "~/renderer-common/hooks/use-persisted-state";
import { assert, iota, items } from "~/shared/utils";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

interface MenuProps {
  side: "left" | "right" | "top" | "bottom";
  children: ReactElement<MenuItemProps> | ReactElement<MenuItemProps>[];
}

export function Menu({ side, children }: Readonly<MenuProps>) {
  // ---- State. --------------------------------------------------------------

  const [size, setSize] = usePersistedState(
    `menu:${side}:size`,
    z.int().min(160).max(640),
    320,
  );

  const [activeItem, setActiveItem] = usePersistedState(
    `menu:${side}:active-item`,
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
  const flexDirection = (() => {
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
    <Flex direction={flexDirection} gap="1px">
      {/* ---- Menu bar. --------------------------------------------------- */}
      <Flex
        align="center"
        justify="between"
        {...(vertical
          ? { direction: "column", height: "100%", pb: "4" }
          : { direction: "row", width: "100%", px: "4" })}
        className={chrome({ direction: "bl" })}
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
                      width="56px"
                      height="56px"
                      onClick={() => {
                        setActiveItemOrToggle(index);
                      }}
                      aria-label={item.props.name}
                      className={cn(
                        "border-l-3 hover:text-(--accent-11)",
                        index === activeItem
                          ? "border-l-(--gray-12) text-(--gray-12) hover:border-l-(--accent-11)"
                          : "border-l-transparent text-(--gray-11)",
                      )}
                    >
                      {item.props.icon}
                    </Flex>
                  </Tooltip>
                ) : (
                  <Fragment key={item.props.name}>
                    {index > 0 && <Separator orientation="vertical" size="1" />}
                    <Flex
                      align="center"
                      justify="center"
                      gap="1"
                      height="32px"
                      onClick={() => {
                        setActiveItemOrToggle(index);
                      }}
                      aria-label={item.props.name}
                      className={cn(
                        "border-b-2 hover:text-(--accent-11)",
                        index === activeItem
                          ? "border-b-(--gray-12) text-(--gray-12) hover:border-b-(--accent-11)"
                          : "border-b-transparent text-(--gray-11)",
                      )}
                    >
                      {item.props.icon}
                      <Text size="1">{item.props.name}</Text>
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
    <Flex
      p="2"
      gap="2"
      height="100%"
      direction="column"
      className={chrome({ direction: "bl" })}
    >
      {/* ---- Header. ----------------------------------------------------- */}
      <Flex gap="2" direction="row" align="center">
        <Box asChild flexGrow="1">
          <Text weight="bold" size="1" truncate>
            {name.toLocaleUpperCase()}
          </Text>
        </Box>

        {actions.map((action) => (
          <Tooltip key={action.name} content={action.name}>
            <IconButton
              size="1"
              variant="ghost"
              color="gray"
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
            size="1"
            variant="ghost"
            color="gray"
            onClick={undefined /** @todo Implement me. */}
            aria-label="Close"
          >
            <MinimizeIcon />
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

Menu.Item = MenuItem;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
