/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { IconBook } from "@tabler/icons-react";

import { Flex } from "~/renderer-common/components/layout";
import { Link } from "~/renderer-common/components/link";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export function HelpMenu() {
  return (
    <Flex direction="column" gap="1" px="3" py="2">
      <Link
        weight="bold"
        href="#"
        target="_blank"
        rel="noreferrer"
        onClick={(event) => {
          event.preventDefault();
          void globalThis.help?.addTab();
        }}
      >
        <IconBook />
        User Manual
      </Link>
    </Flex>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
