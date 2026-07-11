/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import {
  IconDashboard,
  IconDatabase,
  IconHelp,
  IconLogs,
  IconSettings,
  IconTerminal,
} from "@tabler/icons-react";
import { useEffect } from "react";

import { Menu } from "~/renderer/common/components/menu";
import { Window } from "~/renderer/common/components/window";
import { DashboardMenu } from "~/renderer/main/components/dashboard-menu";
import { HelpMenu } from "~/renderer/main/components/help-menu";
import { LogsMenu } from "~/renderer/main/components/logs-menu";
import { SettingsMenu } from "~/renderer/main/components/settings-menu";
import { Timeline } from "~/renderer/main/components/timeline";
import { Viewport } from "~/renderer/main/components/viewport";
import { initSolverState } from "~/renderer/main/state/solver";
import { initStorageState } from "~/renderer/main/state/storage";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export function App() {
  useEffect(() => {
    initSolverState();
    initStorageState();
  }, []);

  return (
    <Window>
      <Page />
    </Window>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

function Page() {
  return (
    <div className="flex size-full min-h-0">
      <Menu.Root
        side="left"
        items={[
          {
            id: "storage",
            group: 0,
            name: "Storage",
            icon: <IconDatabase />,
          },
          {
            id: "dashboard",
            group: 0,
            name: "Dashboard",
            icon: <IconDashboard />,
            content: <DashboardMenu />,
          },
          {
            id: "help",
            group: 1,
            name: "Help",
            icon: <IconHelp />,
            content: <HelpMenu />,
          },
          {
            id: "settings",
            group: 1,
            name: "Settings",
            icon: <IconSettings />,
            content: <SettingsMenu />,
          },
        ]}
      />

      <div className="flex size-full flex-col">
        <div className="min-h-0 grow">
          <Viewport />
        </div>
        <Timeline />

        <Menu.Root
          side="bottom"
          items={[
            {
              id: "logs",
              group: 0,
              name: "Logs",
              icon: <IconLogs />,
              content: <LogsMenu />,
            },
            {
              id: "terminal",
              group: 0,
              name: "Terminal",
              icon: <IconTerminal />,
            },
          ]}
        />
      </div>
    </div>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
