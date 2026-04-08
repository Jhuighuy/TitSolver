/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import type { ReactNode } from "react";

import BlueTitIcon from "~/assets/icon.svg?react";
import { chrome } from "~/renderer-common/components/classes";
import { Box, Flex } from "~/renderer-common/components/layout";
import { Text } from "~/renderer-common/components/text";
import { cn } from "~/renderer-common/components/utils";
import { useWindowIsFullScreen } from "~/renderer-common/hooks/use-window";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

interface WindowProps {
  children: ReactNode;
}

export function Window({ children }: Readonly<WindowProps>) {
  const isFullScreen = useWindowIsFullScreen();

  return (
    <Flex
      direction="column"
      width="100vw"
      height="100vh"
      className="bg-(--bg-1) text-(--fg-1)"
    >
      {/* ---- Title bar. -------------------------------------------------- */}
      {isFullScreen || (
        <Flex
          align="center"
          justify="center"
          gap="1"
          width="100%"
          height="30px"
          minHeight="30px"
          maxHeight="30px"
          className={cn("title-bar-drag", chrome())}
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
      )}

      {/* ---- Content. ---------------------------------------------------- */}
      <Box flexGrow="1" minHeight="0">
        {children}
      </Box>
    </Flex>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
