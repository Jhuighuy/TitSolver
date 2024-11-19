/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import React, { useState } from "react";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export const HorizontalResizableDiv = ({
  children,
  minWidth = 120,
  maxWidth = 720,
  initWidth = 320,
}: {
  children: React.ReactNode;
  minWidth?: number;
  maxWidth?: number;
  initWidth?: number;
}) => {
  const [widthState, setWidthState] = useState(initWidth);

  const resizeHandler = (e: React.MouseEvent) => {
    e.preventDefault();

    // Save the initial mouse position.
    const initX = e.clientX;

    // Update the width of the sidebar once the mouse is moved.
    const onMouseMove = (e: MouseEvent) => {
      const delta = e.clientX - initX;
      const newWidth = Math.max(
        minWidth,
        Math.min(maxWidth, widthState + delta)
      );
      setWidthState(newWidth);
    };

    // Remove the event listener once the mouse is released.
    const onMouseUp = () => {
      // Reset the cursor back to the default.
      document.body.style.cursor = "default";
      // Remove the event listener.
      window.removeEventListener("mousemove", onMouseMove);
      window.removeEventListener("mouseup", onMouseUp);
    };

    // Change the cursor to the resize cursor.
    document.body.style.cursor = "ew-resize";

    // Setup the event listener.
    window.addEventListener("mousemove", onMouseMove);
    window.addEventListener("mouseup", onMouseUp);
  };

  return (
    <div className="flex w-full">
      <div
        className="flex-grow overflow-auto"
        style={{ width: `${widthState}px` }}
      >
        {children}
      </div>
      <div
        className="w-0.5 hover:cursor-ew-resize bg-gray-300"
        onMouseDown={resizeHandler}
      />
    </div>
  );
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export const VerticalResizableDiv = ({
  children,
  minHeight = 120,
  maxHeight = 720,
  initHeight = 320,
}: {
  children: React.ReactNode;
  minHeight?: number;
  maxHeight?: number;
  initHeight?: number;
}) => {
  const [heightState, setHeightState] = useState(initHeight);

  const resizeHandler = (e: React.MouseEvent) => {
    e.preventDefault();

    // Save the initial mouse position.
    const initY = e.clientY;

    // Update the height of the sidebar once the mouse is moved.
    const onMouseMove = (e: MouseEvent) => {
      const delta = e.clientY - initY;
      const newHeight = Math.max(
        minHeight,
        Math.min(maxHeight, heightState + delta)
      );
      setHeightState(newHeight);
    };

    // Remove the event listener once the mouse is released.
    const onMouseUp = () => {
      // Reset the cursor back to the default.
      document.body.style.cursor = "default";
      // Remove the event listener.
      window.removeEventListener("mousemove", onMouseMove);
      window.removeEventListener("mouseup", onMouseUp);
    };

    // Change the cursor to the resize cursor.
    document.body.style.cursor = "ns-resize";

    // Setup the event listener.
    window.addEventListener("mousemove", onMouseMove);
    window.addEventListener("mouseup", onMouseUp);
  };

  return (
    <div className="flex flex-col">
      <div
        className="flex-grow overflow-auto"
        style={{ height: `${heightState}px` }}
      >
        {children}
      </div>
      <div
        className="h-0.5 hover:cursor-ns-resize bg-gray-300"
        onMouseDown={resizeHandler}
      />
    </div>
  );
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
