/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { Box, Flex } from "@radix-ui/themes";
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
import { ViewTimeline } from "~/components/ViewTimeline";
import { Viewport } from "~/components/Viewport";
import { VisualSetup } from "~/components/ViewSetup";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export function App() {
  return (
    <Flex direction="row" width="100vw" height="100vh">
      <Menu side="left" iconSize={24}>
        <Menu.Item name="Setup" icon={GraphIcon} group={0} />
        <Menu.Item name="Storage" icon={DatabaseIcon} group={0} />
        <Menu.Item name="Viewport" icon={PlotIcon} group={1}>
          <VisualSetup />
        </Menu.Item>
        <Menu.Item name="Activity" icon={ActivityIcon} group={1} />
        <Menu.Item name="Settings" icon={SettingsIcon} group={1} />
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
