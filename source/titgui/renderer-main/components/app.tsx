/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import {
  IconDashboard,
  IconDatabase,
  IconHelp,
  IconLogs,
  IconServer,
  IconSettings,
  IconTerminal,
} from "@tabler/icons-react";

import { Box, Flex } from "~/renderer-common/components/layout";
import { Menu } from "~/renderer-common/components/menu";
import { Window } from "~/renderer-common/components/window";
import { ConnectionProvider } from "~/renderer-main/components/connection";
import { DashboardMenu } from "~/renderer-main/components/dashboard-menu";
import { HelpMenu } from "~/renderer-main/components/help-menu";
import { LogsMenu } from "~/renderer-main/components/logs-menu";
import { SettingsMenu } from "~/renderer-main/components/settings-menu";
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
    <Flex size="100%" minHeight="0">
      <Menu.Root side="left">
        <Menu.Item group={0} name="Storage" icon={<IconDatabase />} />
        <Menu.Item group={0} name="Dashboard" icon={<IconDashboard />}>
          <DashboardMenu />
        </Menu.Item>
        <Menu.Item group={1} name="Help" icon={<IconHelp />}>
          <HelpMenu />
        </Menu.Item>
        <Menu.Item group={1} name="Settings" icon={<IconSettings />}>
          <SettingsMenu />
        </Menu.Item>
      </Menu.Root>

      <Flex direction="column" size="100%">
        <Box flexGrow="1" minHeight="0">
          <Viewport />
        </Box>
        <Timeline />

        <Menu.Root side="bottom">
          <Menu.Item group={0} name="Logs" icon={<IconLogs />}>
            <LogsMenu />
          </Menu.Item>
          <Menu.Item group={0} name="Terminal" icon={<IconTerminal />} />
          <Menu.Item group={0} name="Server" icon={<IconServer />} />
        </Menu.Root>
      </Flex>
    </Flex>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
