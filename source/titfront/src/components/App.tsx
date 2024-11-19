/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import React from "react";
import { FiWind, FiPieChart, FiMove } from "react-icons/fi";

import { MyCanvas } from "./view/Canvas";
import { PhysicsPanel } from "./PhysicsPanel";
import { Panel, Sidebar } from "./common/Sidebar";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

const Footer: React.FC = () => {
  return <div className="h-6 bg-gradient-to-t from-gray-100 to-gray-200" />;
};

const Viewport: React.FC = () => {
  return (
    <div className="w-full h-full transition-none">
      <MyCanvas />
    </div>
  );
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export const App: React.FC = () => {
  return (
    <div className="flex flex-col h-screen w-screen select-none bg-gray-100">
      <div className="flex-1 flex">
        {/* Left sidebar. */}
        <Sidebar>
          <Panel label="Scene" icon={<FiMove />}>
            Scene
          </Panel>
          <Panel label="Physics" icon={<FiWind />}>
            <PhysicsPanel />
          </Panel>
          <Panel label="Visual" icon={<FiPieChart />}>
            View
          </Panel>
        </Sidebar>
        <div className="flex-grow relative">
          {/* Main viewport. */}
          <Viewport />
        </div>
      </div>
      {/* Footer. */}
      <Footer />
    </div>
  );
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
