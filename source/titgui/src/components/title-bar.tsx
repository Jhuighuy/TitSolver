/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { Flex, Text } from "@radix-ui/themes";

import BlueTitIcon from "~/assets/icon.svg?react";
import { chrome } from "~/components/classes";
import { cn } from "~/components/utils";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

type TitleBarProps = {
  title?: string;
};

export function TitleBar({ title }: Readonly<TitleBarProps>) {
  const titles = [document.title];
  if (title !== undefined) titles.push(title);

  return (
    <Flex
      direction="row"
      align="center"
      justify="center"
      gap="1"
      width="100%"
      height="30px"
      minHeight="30px"
      maxHeight="30px"
      className={cn("title-bar-drag", chrome({ direction: "br" }))}
    >
      <BlueTitIcon
        width={16}
        height={16}
        aria-label="BlueTit logo"
        role="img"
      />
      <Text size="1" weight="bold">
        {titles.join(" | ")}
      </Text>
    </Flex>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
