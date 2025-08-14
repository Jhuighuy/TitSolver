/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { Box, Flex, Theme } from "@radix-ui/themes";
import { useEffect } from "react";
import { AiOutlinePython as PythonIcon } from "react-icons/ai";
import {
  FiActivity as ActivityIcon,
  FiDatabase as DatabaseIcon,
  FiCamera as PlotIcon,
  FiSettings as SettingsIcon,
  FiTerminal as TerminalIcon,
} from "react-icons/fi";
import { PiGraph as GraphIcon } from "react-icons/pi";

import { Menu } from "~/components/Menu";
import { PythonShell } from "~/components/PythonShell";
import { ConnectionProvider } from "~/components/Server";
import { Settings } from "~/components/Settings";
import { Viewport } from "~/components/Viewport";
import { ViewSetup } from "~/components/ViewSetup";
import { ViewTimeline } from "~/components/ViewTimeline";
import { useSettingsStore } from "~/stores/SettingsStore";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export function App() {
  const { appearance } = useSettingsStore();

  useEffect(() => {
    switch (appearance) {
      case "light":
        document.documentElement.classList.toggle("dark", false);
        break;
      case "dark":
        document.documentElement.classList.toggle("dark", true);
        break;
    }
  }, [appearance]);

  return (
    <ConnectionProvider>
      <Theme appearance={appearance}>
        <Page />
      </Theme>
    </ConnectionProvider>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

function Page() {
  return (
    <Flex direction="row" height="100vh">
      <Menu side="left" iconSize={24}>
        <Menu.Item name="Setup" icon={GraphIcon} group={0} />
        <Menu.Item name="Storage" icon={DatabaseIcon} group={0} />
        <Menu.Item name="Viewport" icon={PlotIcon} group={1}>
          <ViewSetup />
        </Menu.Item>
        <Menu.Item name="Activity" icon={ActivityIcon} group={1} />
        <Menu.Item name="Settings" icon={SettingsIcon} group={1}>
          <Settings />
        </Menu.Item>
      </Menu>
      <Flex flexGrow="1" direction="column">
        <Box flexGrow="1">
          <Viewport />
        </Box>
        <ViewTimeline />
        <Menu side="bottom" iconSize={16}>
          <Menu.Item name="Console" icon={TerminalIcon} group={0} />
          <Menu.Item name="Python shell" icon={PythonIcon} group={0}>
            <PythonShell />
          </Menu.Item>
        </Menu>
      </Flex>
    </Flex>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
