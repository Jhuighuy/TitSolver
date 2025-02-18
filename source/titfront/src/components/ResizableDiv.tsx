/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { FC, ReactNode, useState } from "react";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

interface HorizontalResizableDivProps {
  children: ReactNode;
  minWidth?: number;
  maxWidth?: number;
  initWidth?: number;
}

export const HorizontalResizableDiv: FC<HorizontalResizableDivProps> = ({
  children,
  minWidth = 120,
  maxWidth = 720,
  initWidth = 320,
}) => {
  const [widthState, setWidthState] = useState(initWidth);

  const handleResize = (e: React.MouseEvent) => {
    e.preventDefault();
    const initX = e.clientX;
    const onMouseMove = (e: MouseEvent) => {
      const delta = e.clientX - initX;
      const newWidth = Math.max(
        minWidth,
        Math.min(maxWidth, widthState + delta)
      );
      setWidthState(newWidth);
    };
    const onMouseUp = () => {
      document.body.style.cursor = "default";
      window.removeEventListener("mousemove", onMouseMove);
      window.removeEventListener("mouseup", onMouseUp);
    };
    document.body.style.cursor = "ew-resize";
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
        className="w-0.5 bg-gray-900 hover:cursor-ew-resize"
        onMouseDown={handleResize}
      />
    </div>
  );
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

interface VerticalResizableDivProps {
  children: ReactNode;
  minHeight?: number;
  maxHeight?: number;
  initHeight?: number;
}

export const VerticalResizableDiv: FC<VerticalResizableDivProps> = ({
  children,
  minHeight = 120,
  maxHeight = 720,
  initHeight = 320,
}) => {
  const [heightState, setHeightState] = useState(initHeight);

  const handleResize = (e: React.MouseEvent) => {
    e.preventDefault();
    const initY = e.clientY;
    const onMouseMove = (e: MouseEvent) => {
      const delta = initY - e.clientY;
      const newHeight = Math.max(
        minHeight,
        Math.min(maxHeight, heightState + delta)
      );
      setHeightState(newHeight);
    };
    const onMouseUp = () => {
      document.body.style.cursor = "default";
      window.removeEventListener("mousemove", onMouseMove);
      window.removeEventListener("mouseup", onMouseUp);
    };
    document.body.style.cursor = "ns-resize";
    window.addEventListener("mousemove", onMouseMove);
    window.addEventListener("mouseup", onMouseUp);
  };

  return (
    <div className="flex flex-col">
      <div
        className="h-0.5 hover:cursor-ns-resize bg-gray-900"
        onMouseDown={handleResize}
      />
      <div
        className="flex-grow overflow-auto"
        style={{ height: `${heightState}px` }}
      >
        {children}
      </div>
    </div>
  );
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
