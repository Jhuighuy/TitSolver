/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import type { ReactNode } from "react";

import { Box, Flex } from "~/renderer-common/components/layout";
import { TitleBar } from "~/renderer-common/components/title-bar";
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
      gap="1px"
      width="100vw"
      height="100vh"
      className="bg-(--bg-1) text-(--fg-1)"
    >
      {isFullScreen || <TitleBar />}
      <Box flexGrow="1" minHeight="0">
        {children}
      </Box>
    </Flex>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
