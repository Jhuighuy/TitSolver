/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { Box, Flex, Theme } from "@radix-ui/themes";
import type { ReactNode } from "react";

import { TitleBar } from "~/components/title-bar";
import { useWindowAppearance, useWindowIsFullScreen } from "~/hooks/use-window";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

type WindowProps = {
  title?: string;
  children: ReactNode;
};

export function Window({ title, children }: Readonly<WindowProps>) {
  const appearance = useWindowAppearance();
  const isFullScreen = useWindowIsFullScreen();

  return (
    <Theme appearance={appearance}>
      <Flex direction="column" width="100vw" height="100vh" gap="1px">
        {isFullScreen || <TitleBar title={title} />}
        <Box asChild flexGrow="1" minHeight="0">
          {children}
        </Box>
      </Flex>
    </Theme>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
