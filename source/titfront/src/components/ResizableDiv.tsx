/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { FC, ReactNode } from "react";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

interface HorizontalResizableDivProps {
  children: ReactNode;
  minWidth?: number;
  maxWidth?: number;
  width: number;
  setWidth: (width: number) => void;
}

export const HorizontalResizableDiv: FC<HorizontalResizableDivProps> = ({
  children,
  minWidth = 120,
  maxWidth = 720,
  width,
  setWidth,
}) => {
  const handleResize = (e: React.MouseEvent) => {
    e.preventDefault();
    const initX = e.clientX;
    const defaultCursor = document.body.style.cursor;
    const onMouseMove = (e: MouseEvent) => {
      const delta = e.clientX - initX;
      setWidth(Math.max(minWidth, Math.min(maxWidth, width + delta)));
    };
    const onMouseUp = () => {
      document.body.style.cursor = defaultCursor;
      window.removeEventListener("mousemove", onMouseMove);
      window.removeEventListener("mouseup", onMouseUp);
    };
    document.body.style.cursor = "ew-resize";
    window.addEventListener("mousemove", onMouseMove);
    window.addEventListener("mouseup", onMouseUp);
  };

  return (
    <div className="flex w-full">
      <div className="flex-grow overflow-auto" style={{ width: `${width}px` }}>
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
  height: number;
  setHeight: (height: number) => void;
}

export const VerticalResizableDiv: FC<VerticalResizableDivProps> = ({
  children,
  minHeight = 120,
  maxHeight = 720,
  height,
  setHeight,
}) => {
  const handleResize = (e: React.MouseEvent) => {
    e.preventDefault();
    const initY = e.clientY;
    const defaultCursor = document.body.style.cursor;
    const onMouseMove = (e: MouseEvent) => {
      const delta = initY - e.clientY;
      setHeight(Math.max(minHeight, Math.min(maxHeight, height + delta)));
    };
    const onMouseUp = () => {
      document.body.style.cursor = defaultCursor;
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
        style={{ height: `${height}px` }}
      >
        {children}
      </div>
    </div>
  );
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
