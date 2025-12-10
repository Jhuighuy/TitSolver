/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import {
  Box,
  Flex,
  IconButton,
  ScrollArea,
  Text,
  Tooltip,
} from "@radix-ui/themes";
import { Activity, type ReactElement, type ReactNode, useState } from "react";

import { Resizable } from "~/components/resizable";
import { iota, items } from "~/utils";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

type MenuProps = {
  children: ReactElement<MenuItemProps> | ReactElement<MenuItemProps>[];
};

export function Menu({ children }: Readonly<MenuProps>) {
  // ---- State. --------------------------------------------------------------

  const [size, setSize] = useState(320);
  const [activeItem, setActiveItem] = useState(-1);

  function setActiveItemOrToggle(index: number) {
    setActiveItem((prev) => (index === prev ? -1 : index));
  }

  // ---- Layout. --------------------------------------------------------------

  const childrenArray = items(children);
  const maxGroup = childrenArray.reduce(
    (max, c) => Math.max(max, c.props.group),
    0
  );

  return (
    <Flex direction="row" gap="1px">
      {/* ---- Menu bar. --------------------------------------------------- */}
      <Flex
        align="center"
        justify="between"
        direction="column"
        width="16"
        height="full"
        className="bg-linear-to-bl from-gray-700 to-gray-800  inset-shadow-sm inset-shadow-gray-700"
      >
        {iota(maxGroup + 1).map((group) => (
          <Flex key={group} align="center" m="2" gap="4" direction="column">
            {childrenArray.map(
              (item, index) =>
                item.props.group === group && (
                  <Tooltip
                    key={item.props.name}
                    content={item.props.name}
                    side="right"
                  >
                    <IconButton
                      aria-label={item.props.name}
                      size="4"
                      variant={index === activeItem ? "classic" : "outline"}
                      onClick={() => setActiveItemOrToggle(index)}
                    >
                      {item.props.icon}
                    </IconButton>
                  </Tooltip>
                )
            )}
          </Flex>
        ))}
      </Flex>

      {/* ---- Menu item. -------------------------------------------------- */}
      <Activity mode={activeItem === -1 ? "hidden" : "visible"}>
        <Resizable minSize={160} maxSize={640} size={size} setSize={setSize}>
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

type MenuItemProps = {
  group: number;
  name: string;
  icon: ReactElement;
  children?: ReactNode;
};

function MenuItem({ name, children }: Readonly<MenuItemProps>) {
  return (
    <Flex
      p="2"
      gap="2"
      height="100%"
      direction="column"
      className="bg-linear-to-bl from-gray-700 to-gray-800 inset-shadow-sm inset-shadow-gray-700"
    >
      {/* ---- Header. ----------------------------------------------------- */}
      {/** @todo Add user-configurable actions. */}
      <Flex gap="2" direction="row" align="center">
        <Box asChild flexGrow="1">
          <Text weight="bold" size="2" truncate>
            {name.toLocaleUpperCase()}
          </Text>
        </Box>
      </Flex>

      {/* ---- Contents. --------------------------------------------------- */}
      <Box
        flexGrow="1"
        overflow="auto"
        px="2"
        py="1"
        className="bg-gray-900 rounded-lg"
      >
        <ScrollArea>{children}</ScrollArea>
      </Box>
    </Flex>
  );
}

Menu.Item = MenuItem;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
