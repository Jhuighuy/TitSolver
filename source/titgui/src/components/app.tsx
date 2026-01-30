/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { QueryClient, QueryClientProvider } from "@tanstack/react-query";
import { Box, Flex, Theme } from "@radix-ui/themes";
import { TbHelp as HelpIcon, TbRun as RunIcon } from "react-icons/tb";

import { Menu } from "~/components/menu";
import { HelpMenu } from "~/components/menu-help";
import { RunMenu } from "~/components/menu-run";
import { Timeline } from "~/components/timeline";
import { Viewport } from "~/components/viewport";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export function App() {
  return (
    <QueryClientProvider client={queryClient}>
      <Theme appearance="dark">
        <Page />
      </Theme>
    </QueryClientProvider>
  );
}

const queryClient = new QueryClient();

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

function Page() {
  return (
    <Flex direction="row" height="100vh" gap="1px">
      <Menu>
        <Menu.Item group={0} name="Run" icon={<RunIcon size={32} />}>
          <RunMenu />
        </Menu.Item>
        <Menu.Item group={1} name="Help" icon={<HelpIcon size={32} />}>
          <HelpMenu />
        </Menu.Item>
      </Menu>

      <Flex direction="column" width="100%" height="100%" gap="1px">
        <Box flexGrow="1">
          <Viewport />
        </Box>
        <Timeline />
      </Flex>
    </Flex>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
