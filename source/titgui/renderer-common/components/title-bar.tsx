/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import BlueTitIcon from "~/assets/icon.svg?react";
import { chrome } from "~/renderer-common/components/classes";
import { Flex } from "~/renderer-common/components/layout";
import { Text } from "~/renderer-common/components/text";
import { cn } from "~/renderer-common/components/utils";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export function TitleBar() {
  return (
    <Flex
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
      <Text size="2" weight="bold">
        {document.title}
      </Text>
    </Flex>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
