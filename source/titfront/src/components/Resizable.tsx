/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import type { FC, ReactNode } from "react";

import { cn, type Side } from "~/utils";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

interface ResizableDivProps {
  side: Side;
  size: number;
  setSize: (size: number) => void;
  minSize?: number;
  maxSize?: number;
  children: ReactNode;
}

export const Resizable: FC<ResizableDivProps> = ({
  side,
  size,
  setSize,
  minSize = 120,
  maxSize = 720,
  children,
}) => {
  const horizontal = side === "left" || side === "right";
  const reversed = side === "right" || side === "bottom";
  const cursor = horizontal ? "ew-resize" : "ns-resize";
  const dimension = horizontal ? "width" : "height";
  const axisProp = horizontal ? "clientX" : "clientY";
  const flexDirection = {
    left: "flex-row",
    right: "flex-row-reverse",
    top: "flex-col",
    bottom: "flex-col-reverse",
  }[side];

  const handleMouseDown = (event: React.MouseEvent) => {
    event.preventDefault();
    const prevCursor = document.body.style.cursor;
    const onMouseMove = (moveEvent: MouseEvent) => {
      const delta = moveEvent[axisProp] - event[axisProp];
      const newSize = reversed ? size - delta : size + delta;
      setSize(Math.max(minSize, Math.min(newSize, maxSize)));
    };
    const onMouseUp = () => {
      document.body.style.cursor = prevCursor;
      globalThis.removeEventListener("mousemove", onMouseMove);
      globalThis.removeEventListener("mouseup", onMouseUp);
    };
    document.body.style.cursor = cursor;
    globalThis.addEventListener("mousemove", onMouseMove);
    globalThis.addEventListener("mouseup", onMouseUp);
  };

  return (
    <div className={cn("flex", flexDirection)}>
      <div
        data-testid="resizable-content"
        className="flex-grow size-full overflow-auto"
        style={{ [dimension]: `${size}px` }}
      >
        {children}
      </div>
      <button
        data-testid="resizable-resizer"
        type="button"
        className="bg-gray-900"
        style={{ cursor: cursor, [dimension]: "2px" }}
        onMouseDown={handleMouseDown}
      />
    </div>
  );
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
