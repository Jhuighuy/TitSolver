/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { Box, Flex } from "@radix-ui/themes";
import {
  TbDashboard as DashboardIcon,
  TbHelp as HelpIcon,
  TbLogs as LogsIcon,
  TbServer as ServerIcon,
  TbSettings as SettingsIcon,
  TbDatabase as StorageIcon,
  TbTerminal as TerminalIcon,
} from "react-icons/tb";

import { Window } from "~/renderer-common/components/window";
import { ConnectionProvider } from "~/renderer-main/components/connection";
import { Menu } from "~/renderer-main/components/menu";
import { HelpMenu } from "~/renderer-main/components/menu-help";
import { LogsMenu } from "~/renderer-main/components/menu-logs";
import { RunMenu } from "~/renderer-main/components/menu-run";
import { SettingsMenu } from "~/renderer-main/components/menu-settings";
import { SolverProvider } from "~/renderer-main/components/solver";
import { StorageProvider } from "~/renderer-main/components/storage";
import { Timeline } from "~/renderer-main/components/timeline";
import { Viewport } from "~/renderer-main/components/viewport";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export function App() {
  return (
    <ConnectionProvider>
      <SolverProvider>
        <StorageProvider>
          <Window>
            <Page />
          </Window>
        </StorageProvider>
      </SolverProvider>
    </ConnectionProvider>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

function Page() {
  return (
    <Flex direction="row" width="100%" height="100%" minHeight="0" gap="1px">
      <Menu side="left">
        <Menu.Item group={0} name="Storage" icon={<StorageIcon size={32} />} />
        <Menu.Item
          group={0}
          name="Dashboard"
          icon={<DashboardIcon size={32} />}
        >
          <RunMenu />
        </Menu.Item>
        <Menu.Item group={1} name="Help" icon={<HelpIcon size={32} />}>
          <HelpMenu />
        </Menu.Item>
        <Menu.Item group={1} name="Settings" icon={<SettingsIcon size={32} />}>
          <SettingsMenu />
        </Menu.Item>
      </Menu>

      <Flex direction="column" width="100%" height="100%" gap="1px">
        <Box flexGrow="1">
          <Viewport />
        </Box>
        <Timeline />

        <Menu side="bottom">
          <Menu.Item group={0} name="Logs" icon={<LogsIcon size={16} />}>
            <LogsMenu />
          </Menu.Item>
          <Menu.Item
            group={0}
            name="Terminal"
            icon={<TerminalIcon size={16} />}
          />
          <Menu.Item group={0} name="Server" icon={<ServerIcon size={16} />} />
        </Menu>
      </Flex>
    </Flex>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
