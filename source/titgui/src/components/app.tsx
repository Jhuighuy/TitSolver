/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { Box, Flex, Theme } from "@radix-ui/themes";
import {
  TbDashboard as DashboardIcon,
  TbHelp as HelpIcon,
  TbLogs as LogsIcon,
  TbServer as ServerIcon,
  TbSettings as SettingsIcon,
  TbDatabase as StorageIcon,
  TbTerminal as TerminalIcon,
  TbTable as TableIcon,
} from "react-icons/tb";

import { ConnectionProvider } from "~/components/connection";
import { Menu } from "~/components/menu";
import { HelpMenu } from "~/components/menu-help";
import { LogsMenu } from "~/components/menu-logs";
import { RunMenu } from "~/components/menu-run";
import { SettingsMenu } from "~/components/menu-settings";
import { TableMenu } from "~/components/menu-table";
import { SelectionProvider } from "~/components/selection";
import { SolverProvider } from "~/components/solver";
import { StorageProvider } from "~/components/storage";
import { Timeline } from "~/components/timeline";
import { TitleBar } from "~/components/title-bar";
import { Viewport } from "~/components/viewport";
import { useWindowAppearance, useWindowIsFullScreen } from "~/hooks/use-window";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export function App() {
  return (
    <ConnectionProvider>
      <SolverProvider>
        <StorageProvider>
          <SelectionProvider>
            <Window />
          </SelectionProvider>
        </StorageProvider>
      </SolverProvider>
    </ConnectionProvider>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

function Window() {
  const appearance = useWindowAppearance();
  const isFullScreen = useWindowIsFullScreen();
  return (
    <Theme appearance={appearance}>
      <Flex direction="column" width="100vw" height="100vh" gap="1px">
        {isFullScreen || <TitleBar />}
        <Box asChild flexGrow="1">
          <Page />
        </Box>
      </Flex>
    </Theme>
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

      <Flex
        direction="column"
        width="100%"
        height="100%"
        minWidth="0"
        minHeight="0"
        gap="1px"
      >
        <Box flexGrow="1" minWidth="0" minHeight="0">
          <Viewport />
        </Box>
        <Timeline />

        <Menu side="bottom">
          <Menu.Item group={0} name="Particles" icon={<TableIcon size={16} />}>
            <TableMenu />
          </Menu.Item>
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
