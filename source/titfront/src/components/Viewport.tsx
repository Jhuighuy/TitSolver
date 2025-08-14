/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { DropdownMenu, Flex, IconButton, Select } from "@radix-ui/themes";
import { useState } from "react";
import { FaCaretDown as DownIcon } from "react-icons/fa";

import { ViewPlot2D } from "~/components/ViewPlot2D";
import { ViewPlot3D } from "~/components/ViewPlot3D";
import { iota } from "~/utils";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export type MultiViewportLayout = "1" | "1x2" | "2x1" | "(1+1)x1" | "2x2";

export function Viewport() {
  const [layout, setLayout] = useState<MultiViewportLayout>("1");

  const grid = {
    "1": [[1]],
    "1x2": [[2]],
    "2x1": [[1], [1]],
    "(1+1)x1": [[2], [1]],
    "2x2": [[2], [2]],
  }[layout];

  return (
    <Flex
      direction="column"
      width="100%"
      height="100%"
      gap="2px"
      pr="2px"
      py="2px"
      className="bg-gray-900"
    >
      {grid.map((row, i) => (
        <Flex
          // biome-ignore lint/suspicious/noArrayIndexKey: .
          key={i}
          direction="row"
          gap="2px"
          height={`${100 / grid.length}%`}
        >
          {iota(row[0]).map((j) => (
            <Flex asChild key={j} direction="row" gap="2">
              <ViewportCell setLayout={setLayout} />
            </Flex>
          ))}
        </Flex>
      ))}
    </Flex>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

function ViewportCell({
  setLayout,
}: {
  setLayout: (layout: MultiViewportLayout) => void;
}) {
  const [displayMode, setDisplayMode] = useState<"2D" | "3D">("3D");

  return (
    <Flex width="100%" height="100%" direction="column" className="rounded">
      <Flex
        width="100%"
        height="24px"
        px="2"
        py="1"
        direction="row"
        align="center"
        justify="between"
        className="bg-gradient-to-b from-gray-800 to-gray-900"
      >
        <Select.Root
          size="1"
          value={displayMode}
          onValueChange={(x) => setDisplayMode(x as "2D" | "3D")}
        >
          <Select.Trigger variant="ghost" color="gray" />
          <Select.Content position="popper">
            <Select.Item value="2D">2D</Select.Item>
            <Select.Item value="3D">3D</Select.Item>
          </Select.Content>
        </Select.Root>
        <DropdownMenu.Root>
          <DropdownMenu.Trigger>
            <IconButton variant="ghost" color="gray">
              <DownIcon size={8} />
            </IconButton>
          </DropdownMenu.Trigger>
          <DropdownMenu.Content>
            <DropdownMenu.Item onClick={() => setLayout("1")}>
              1
            </DropdownMenu.Item>
            <DropdownMenu.Item onClick={() => setLayout("1x2")}>
              1x2
            </DropdownMenu.Item>
            <DropdownMenu.Item onClick={() => setLayout("2x1")}>
              2x1
            </DropdownMenu.Item>
            <DropdownMenu.Item onClick={() => setLayout("(1+1)x1")}>
              (1+1)x1
            </DropdownMenu.Item>
            <DropdownMenu.Item onClick={() => setLayout("2x2")}>
              2x2
            </DropdownMenu.Item>
          </DropdownMenu.Content>
        </DropdownMenu.Root>
      </Flex>
      {displayMode === "2D" && <ViewPlot2D />}
      {displayMode === "3D" && <ViewPlot3D />}
    </Flex>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
