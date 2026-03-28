/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { Box, Flex, Theme } from "@radix-ui/themes";
import type { ReactNode } from "react";

import { TitleBar } from "~/renderer-common/components/title-bar";
import {
  useWindowIsFullScreen,
  useWindowTheme,
} from "~/renderer-common/hooks/use-window";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

interface WindowProps {
  children: ReactNode;
}

export function Window({ children }: Readonly<WindowProps>) {
  const theme = useWindowTheme();
  const isFullScreen = useWindowIsFullScreen();

  return (
    <Theme appearance={theme}>
      <Flex direction="column" width="100vw" height="100vh" gap="1px">
        {isFullScreen || <TitleBar />}
        <Box asChild flexGrow="1" minHeight="0">
          {children}
        </Box>
      </Flex>
    </Theme>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
