/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { Box, Flex, Theme } from "@radix-ui/themes";
import {
  TbHelp as HelpIcon,
  TbTableOptions as ParamsIcon,
  TbRun as RunIcon,
} from "react-icons/tb";

import { ConnectionProvider } from "~/components/connection";
import { Menu } from "~/components/menu";
import { HelpMenu } from "~/components/menu-help";
import { ParamsMenu } from "~/components/menu-params";
import { RunMenu } from "~/components/menu-run";
import { SolverProvider } from "~/components/solver";
import { StorageProvider } from "~/components/storage";
import { Timeline } from "~/components/timeline";
import { Viewport } from "~/components/viewport";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export function App() {
  return (
    <Theme appearance="dark">
      <ConnectionProvider>
        <SolverProvider>
          <StorageProvider>
            <Page />
          </StorageProvider>
        </SolverProvider>
      </ConnectionProvider>
    </Theme>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

function Page() {
  return (
    <Flex direction="row" height="100vh" gap="1px">
      <Menu>
        <Menu.Item group={0} name="Parameters" icon={<ParamsIcon size={32} />}>
          <ParamsMenu />
        </Menu.Item>
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
