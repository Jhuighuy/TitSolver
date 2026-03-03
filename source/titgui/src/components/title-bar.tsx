/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { Flex, Text } from "@radix-ui/themes";
import BlueTitIcon from "~/assets/icon.svg?react";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export function TitleBar() {
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
      className="title-bar-drag bg-linear-to-br from-gray-700 to-gray-800"
    >
      <BlueTitIcon
        width={16}
        height={16}
        aria-label="BlueTit logo"
        role="img"
      />
      <Text size="1" weight="bold">
        {document.title}
      </Text>
    </Flex>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
