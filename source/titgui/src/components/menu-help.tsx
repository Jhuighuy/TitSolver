/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { Button, Flex } from "@radix-ui/themes";
import { TbBook as BookIcon } from "react-icons/tb";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export function HelpMenu() {
  return (
    <Flex direction="column" gap="1" px="3" py="2">
      <Button
        variant="ghost"
        highContrast
        size="2"
        onClick={() => void window.help?.open()}
        style={{ justifyContent: "flex-start" }}
      >
        <Flex align="center" gap="2">
          <BookIcon size={16} />
          User Manual
        </Flex>
      </Button>
    </Flex>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
