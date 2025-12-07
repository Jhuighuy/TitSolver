/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { Box, Flex, Theme } from "@radix-ui/themes";

import { ConnectionProvider } from "~/components/connection";
import { StorageProvider } from "~/components/storage";
import { Timeline } from "~/components/timeline";
import { Viewport } from "~/components/viewport";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export function App() {
  return (
    <Theme appearance="dark">
      <ConnectionProvider>
        <StorageProvider>
          <Page />
        </StorageProvider>
      </ConnectionProvider>
    </Theme>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

function Page() {
  return (
    <Flex direction="column" height="100vh" gap="1px">
      <Box flexGrow="1">
        <Viewport />
      </Box>
      <Timeline />
    </Flex>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
