/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import type { ReactNode } from "react";

import { Box, Flex } from "~/renderer-common/components/layout";
import { cn } from "~/renderer-common/components/utils";
import { VisuallyHidden } from "~/renderer-common/components/visually-hidden";
import { clamp } from "~/renderer-common/utils-math";
import { assert } from "~/shared/utils";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

interface ResizableDivProps {
  side: "left" | "right" | "top" | "bottom";
  size: number;
  setSize: (size: number) => void;
  minSize: number;
  maxSize: number;
  children?: ReactNode;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export function Resizable({
  side,
  size,
  setSize,
  minSize,
  maxSize,
  children,
}: Readonly<ResizableDivProps>) {
  assert(0 < minSize && minSize <= size && size <= maxSize);

  const reverse = side === "right" || side === "bottom";
  const horizontal = side === "left" || side === "right";
  const direction = (() => {
    switch (side) {
      case "left":
        return "row";
      case "right":
        return "row-reverse";
      case "top":
        return "column";
      case "bottom":
        return "column-reverse";
    }
  })();

  function updateSize(delta: number) {
    const newSize = reverse ? size - delta : size + delta;
    setSize(clamp(newSize, minSize, maxSize));
  }

  function handleMouseDown(downEvent: React.MouseEvent) {
    downEvent.preventDefault();

    const prevCursor = document.body.style.cursor;
    document.body.style.cursor = horizontal ? "ew-resize" : "ns-resize";

    function handleMouseMove(moveEvent: MouseEvent) {
      const delta = horizontal
        ? moveEvent.clientX - downEvent.clientX
        : moveEvent.clientY - downEvent.clientY;
      updateSize(delta);
    }

    function handleMouseUp() {
      document.body.style.cursor = prevCursor;
      globalThis.removeEventListener("mousemove", handleMouseMove);
      globalThis.removeEventListener("mouseup", handleMouseUp);
    }

    globalThis.addEventListener("mousemove", handleMouseMove);
    globalThis.addEventListener("mouseup", handleMouseUp);
  }

  function handleKeyDown(event: React.KeyboardEvent) {
    const delta = 10;
    switch (event.key) {
      case horizontal ? "ArrowLeft" : "ArrowUp":
        event.preventDefault();
        updateSize(-delta);
        break;
      case horizontal ? "ArrowRight" : "ArrowDown":
        event.preventDefault();
        updateSize(delta);
        break;
      default:
        return;
    }
  }

  return (
    <Flex direction={direction}>
      <Box
        {...(horizontal
          ? { width: `${size}px`, height: "100%" }
          : { height: `${size}px`, width: "100%" })}
      >
        {children}
      </Box>
      <Box position="relative">
        <Box
          tabIndex={0}
          onMouseDown={handleMouseDown}
          onKeyDown={handleKeyDown}
          role="separator"
          aria-label="Resize panel"
          aria-orientation={horizontal ? "horizontal" : "vertical"}
          aria-valuemin={minSize}
          aria-valuemax={maxSize}
          aria-valuenow={size}
          position="absolute"
          {...(horizontal
            ? { left: "-1px", width: "2px", height: "100%" }
            : { top: "-1px", width: "100%", height: "2px" })}
          className={cn(
            "focus-visible:outline-2 focus-visible:outline-(--accent-fg-3)",
            horizontal ? "cursor-ew-resize" : "cursor-ns-resize",
          )}
        >
          <VisuallyHidden>Resize</VisuallyHidden>
        </Box>
      </Box>
    </Flex>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
