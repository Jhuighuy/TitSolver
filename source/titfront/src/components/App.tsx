/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import type { FC } from "react";
import { AiOutlinePython as PythonIcon } from "react-icons/ai";
import {
  FiActivity as ActivityIcon,
  FiDatabase as DatabaseIcon,
  FiSettings as SettingsIcon,
  FiSliders as SlidersIcon,
  FiTerminal as TerminalIcon,
} from "react-icons/fi";

import { Menu } from "~/components/Menu";
import { Viewer } from "~/components/Viewer";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export const App: FC = () => {
  const leftIconSize = 24;
  const bottomIconSize = 16;
  return (
    <div className="h-screen w-screen flex flex-row select-none text-sm">
      <Menu side="left">
        <Menu.Item
          name="Configuration"
          icon={<SlidersIcon size={leftIconSize} />}
          group={0}
        />
        <Menu.Item
          name="Storage"
          icon={<DatabaseIcon size={leftIconSize} />}
          group={0}
        />
        <Menu.Item
          name="Activity"
          icon={<ActivityIcon size={leftIconSize} />}
          group={1}
        />
        <Menu.Item
          name="Settings"
          icon={<SettingsIcon size={leftIconSize} />}
          group={1}
        />
      </Menu>
      <div className="flex-1 flex flex-col">
        <Viewer />
        <Menu side="bottom">
          <Menu.Item
            name="Python shell"
            icon={<PythonIcon size={bottomIconSize} />}
            group={0}
          />
          <Menu.Item
            name="Console"
            icon={<TerminalIcon size={bottomIconSize} />}
            group={0}
          />
        </Menu>
      </div>
    </div>
  );
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
