/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import {
  Box,
  Button,
  Flex,
  IconButton,
  ScrollArea,
  Separator,
  Text,
} from "@radix-ui/themes";
import {
  createContext,
  Fragment,
  type ReactElement,
  useContext,
  useEffect,
  useState,
} from "react";
import type { IconType } from "react-icons";
import { FiMinimize as MinimizeIcon } from "react-icons/fi";

import { Resizable } from "~/components/Resizable";
import { useMenuStore } from "~/stores/LayoutStore";
import {
  assert,
  iota,
  oppositeOrientation,
  orientationToFlexDirection,
  type Side,
  sideToFlexDirection,
  sideToOrientation,
} from "~/utils";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

type Action = {
  key: string;
  icon: IconType;
  callback: () => void;
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

type MenuProviderProps = {
  addAction: (action: Action) => void;
  removeAction: (key: string) => void;
};

const MenuContext = createContext<MenuProviderProps | null>(null);

export function useMenuAction(action: Action) {
  const context = useContext(MenuContext);
  assert(context !== null, "`useMenuAction` called outside `Menu`!");
  const { addAction, removeAction } = context;
  useEffect(() => {
    addAction(action);
    return () => removeAction(action.key);
  }, [action, addAction, removeAction]);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

type MenuItemProps = {
  name: string;
  icon: IconType;
  group: number;
  children?: ReactElement;
};

function MenuItem({ children }: MenuItemProps) {
  return <>{children}</>;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

type MenuProps = {
  side: Side;
  iconSize: number;
  children: ReactElement<MenuItemProps>[];
};

export function Menu({ side, iconSize, children }: MenuProps) {
  // ---- Actions control. -----------------------------------------------------

  const [userActions, setUserActions] = useState<Action[]>([]);

  const actions = {
    addAction(action: Action) {
      setUserActions((prev) => [...prev, action]);
    },
    removeAction(key: string) {
      setUserActions((prev) => prev.filter((action) => action.key !== key));
    },
  };

  // ---- Layout. --------------------------------------------------------------

  const { size, setSize, activeItem, setActiveItem } = useMenuStore(side);
  const orientation = oppositeOrientation(sideToOrientation(side));
  const maxGroup = children.reduce((max, c) => Math.max(max, c.props.group), 0);

  return (
    <Flex direction={sideToFlexDirection(side)} gap="1px">
      {/* ---- Menu bar. --------------------------------------------------- */}
      <Flex
        align="center"
        justify="between"
        direction={orientationToFlexDirection(orientation)}
        width={orientation === "vertical" ? "16" : "full"}
        height={orientation === "vertical" ? "full" : "8"}
        className="bg-gradient-to-bl from-gray-700 to-gray-800  inset-shadow-sm inset-shadow-gray-700"
      >
        {/* ---- Item groups. ---------------------------------------------- */}
        {iota(maxGroup + 1).map((group) => (
          <Flex
            key={group}
            align="center"
            m="2"
            gap={orientation === "vertical" ? "4" : "1"}
            direction={orientationToFlexDirection(orientation)}
          >
            {/* ---- Items. ------------------------------------------------ */}
            {children.map(
              (item, index) =>
                item.props.group === group &&
                (orientation === "vertical" ? (
                  <IconButton
                    key={item.props.name}
                    size="4"
                    variant={index === activeItem ? "classic" : "outline"}
                    onClick={() => setActiveItem(index)}
                  >
                    <item.props.icon size={iconSize} />
                  </IconButton>
                ) : (
                  <Fragment key={item.props.name}>
                    {index > 0 && <Separator orientation="vertical" />}
                    <Button
                      size="1"
                      radius="large"
                      variant={index === activeItem ? "classic" : "outline"}
                      style={{ minWidth: "8rem" }}
                      onClick={() => setActiveItem(index)}
                    >
                      <item.props.icon size={iconSize} />
                      <Text>{item.props.name}</Text>
                    </Button>
                  </Fragment>
                ))
            )}
          </Flex>
        ))}
      </Flex>

      {/* ---- Active item. ------------------------------------------------ */}
      {activeItem !== -1 && (
        <Resizable side={side} size={size} setSize={setSize}>
          <Flex
            p="2"
            gap="2"
            height="100%"
            direction="column"
            className="bg-gradient-to-bl from-gray-700 to-gray-800 inset-shadow-sm inset-shadow-gray-700"
          >
            {/* ---- Header. ----------------------------------------------- */}
            <Flex gap="2" direction="row" align="center">
              {/* Title. */}
              <Box asChild flexGrow="1">
                <Text weight="bold" size="2" truncate>
                  {children[activeItem].props.name.toLocaleUpperCase()}
                </Text>
              </Box>

              {/* User actions. */}
              {userActions.map((action) => (
                <IconButton
                  key={action.key}
                  size="1"
                  variant="ghost"
                  color="gray"
                  onClick={action.callback}
                >
                  <action.icon size={16} />
                </IconButton>
              ))}

              {/* Built-in actions. */}
              <IconButton
                size="1"
                variant="ghost"
                color="gray"
                onClick={() => setActiveItem(-1)}
              >
                <MinimizeIcon size={16} />
              </IconButton>
            </Flex>

            {/* ---- Contents. --------------------------------------------- */}
            <Box
              flexGrow="1"
              overflow="auto"
              className="bg-gray-900 rounded-lg"
            >
              <ScrollArea>
                <MenuContext.Provider value={actions}>
                  {children[activeItem]}
                </MenuContext.Provider>
              </ScrollArea>
            </Box>
          </Flex>
        </Resizable>
      )}
    </Flex>
  );
}

Menu.Item = MenuItem;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
