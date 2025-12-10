/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { Box, Flex, VisuallyHidden } from "@radix-ui/themes";
import type { ReactNode } from "react";

import { assert } from "~/utils";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

type ResizableDivProps = {
  minSize: number;
  maxSize: number;
  size: number;
  setSize: (size: number) => void;
  children?: ReactNode;
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export function Resizable({
  size,
  minSize,
  maxSize,
  setSize,
  children,
}: Readonly<ResizableDivProps>) {
  assert(0 < minSize && minSize <= size && size <= maxSize);

  function setSizeClamped(newSize: number) {
    setSize(Math.max(minSize, Math.min(newSize, maxSize)));
  }

  function handleMouseDown(downEvent: React.MouseEvent) {
    downEvent.preventDefault();

    const prevCursor = document.body.style.cursor;
    document.body.style.cursor = "ew-resize";

    function handleMouseMove(moveEvent: MouseEvent) {
      const delta = moveEvent.clientX - downEvent.clientX;
      setSizeClamped(size + delta);
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
      case "ArrowLeft":
        event.preventDefault();
        setSizeClamped(size - delta);
        break;
      case "ArrowRight":
        event.preventDefault();
        setSizeClamped(size + delta);
        break;
      default:
        return;
    }
  }

  return (
    <Flex direction="row">
      <div
        style={{
          width: `${size}px`,
          height: "100%",
        }}
      >
        {children}
      </div>
      <Box
        className="bg-gray-700 focus-visible:outline-(--accent-8)"
        width="2px"
        tabIndex={0}
        onMouseDown={handleMouseDown}
        onKeyDown={handleKeyDown}
        style={{ cursor: "ew-resize" }}
        role="separator"
        aria-label="Resize panel"
        aria-orientation="horizontal"
        aria-valuemin={minSize}
        aria-valuemax={maxSize}
        aria-valuenow={size}
      >
        <VisuallyHidden>Resize</VisuallyHidden>
      </Box>
    </Flex>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
