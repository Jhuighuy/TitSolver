/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { Box, Flex, VisuallyHidden } from "@radix-ui/themes";
import type { ReactNode } from "react";
import { type Side, sideToFlexDirection } from "~/utils";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

type ResizableDivProps = {
  side: Side;
  size: number;
  setSize: (size: number) => void;
  minSize?: number;
  maxSize?: number;
  children?: ReactNode;
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export function Resizable({
  side,
  size,
  setSize,
  minSize = 120,
  maxSize = 720,
  children,
}: ResizableDivProps) {
  const horizontal = side === "left" || side === "right";
  const reversed = side === "right" || side === "bottom";

  const cursor = horizontal ? "ew-resize" : "ns-resize";
  const dimension = horizontal ? "width" : "height";
  const axisProp = horizontal ? "clientX" : "clientY";

  if (size < minSize) setSize(minSize);
  if (size > maxSize) setSize(maxSize);

  function handleMouseDown(downEvent: React.MouseEvent) {
    downEvent.preventDefault();

    const prevCursor = document.body.style.cursor;
    document.body.style.cursor = cursor;

    function handleMouseMove(moveEvent: MouseEvent) {
      const delta = moveEvent[axisProp] - downEvent[axisProp];
      const newSize = reversed ? size - delta : size + delta;
      setSize(Math.max(minSize, Math.min(newSize, maxSize)));
    }

    function handleMouseUp() {
      document.body.style.cursor = prevCursor;
      window.removeEventListener("mousemove", handleMouseMove);
      window.removeEventListener("mouseup", handleMouseUp);
    }

    window.addEventListener("mousemove", handleMouseMove);
    window.addEventListener("mouseup", handleMouseUp);
  }

  return (
    <Flex direction={sideToFlexDirection(side)}>
      <div
        data-testid="resizable-content"
        style={{
          width: horizontal ? `${size}px` : "full",
          height: horizontal ? "full" : `${size}px`,
        }}
      >
        {children}
      </div>
      <Box
        data-testid="resizable-resizer"
        className="bg-gray-700"
        style={{ cursor: cursor, [dimension]: "2px" }}
        onMouseDown={handleMouseDown}
      >
        <VisuallyHidden>Resize</VisuallyHidden>
      </Box>
    </Flex>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
