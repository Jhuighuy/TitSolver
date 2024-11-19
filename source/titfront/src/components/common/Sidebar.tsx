/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import React, { useState } from "react";

import { HorizontalResizableDiv, VerticalResizableDiv } from "./ResizableDiv";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

interface TabProps {
  label: string;
  end?: boolean;
  icon: React.ReactNode;
  children: React.ReactNode;
}

export const Panel: React.FC<TabProps> = ({ children }) => {
  return <div>{children}</div>;
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export const Sidebar = ({
  children,
}: {
  children: React.ReactElement<TabProps>[];
}) => {
  // Handle the active panel.
  const [activePanelIndex, setActivePanelIndex] = useState(1);

  const togglePanel = (index: number) => {
    setActivePanelIndex((prev) => (prev === index ? -1 : index));
  };

  return (
    <div className="flex h-full">
      {/* Sidebar panel icons. */}
      <div className="w-12 pt-2 flex flex-col items-center shadow-inner bg-gradient-to-r from-gray-100 to-gray-200">
        {children.map((panel, index) => (
          <div
            key={index}
            className={`p-2 mb-4 w-10 h-10 flex items-center justify-center
                        rounded text-6xl text-gray-800 ${
                          index === activePanelIndex
                            ? "bg-gray-400 shadow text-black"
                            : "hover:bg-gray-300"
                        }`}
            onClick={() => togglePanel(index)}
          >
            {panel.props.icon}
          </div>
        ))}
      </div>
      {/* Sidebar active panel. */}
      {activePanelIndex !== -1 && (
        <HorizontalResizableDiv>
          <div className="flex items-center h-10 border-b-2 border-gray-300 bg-gray-200">
            <span className="p-3 text-sm font-bold cursor-default">
              {children[activePanelIndex].props.label}
            </span>
          </div>
          <VerticalResizableDiv>
            <div className="p-1 inner-shadow-xl">
              {children[activePanelIndex]}
            </div>
          </VerticalResizableDiv>
        </HorizontalResizableDiv>
      )}
    </div>
  );
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
