/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import type { FC } from "react";
import {
  FiActivity as ActivityIcon,
  FiDatabase as DatabaseIcon,
  FiSettings as SettingsIcon,
  FiSliders as SlidersIcon,
} from "react-icons/fi";

import { Menu } from "~/components/Menu";
import { MockData, TreeView } from "~/components/TreeView";
import { ViewerComponent } from "~/components/Viewer";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export const App: FC = () => {
  const leftIconSize = 24;
  return (
    <div className="h-screen w-screen flex flex-row select-none text-sm">
      <Menu side="left">
        <Menu.Item
          name="Configuration"
          icon={<SlidersIcon size={leftIconSize} />}
          group={0}
        >
          <TreeView nodes={MockData} />
        </Menu.Item>
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
        <ViewerComponent />
      </div>
    </div>
  );
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
