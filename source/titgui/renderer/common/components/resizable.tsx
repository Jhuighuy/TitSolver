/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import type { ReactNode } from "react";

import { cn } from "~/renderer/common/components/utils";
import { VisuallyHidden } from "~/renderer/common/components/visually-hidden";
import { clamp } from "~/renderer/common/utils-math";
import { assert } from "~/shared/utils";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

const directionClasses = {
  row: "flex-row",
  "row-reverse": "flex-row-reverse",
  column: "flex-col",
  "column-reverse": "flex-col-reverse",
} as const;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

interface ResizableProps {
  side: "left" | "right" | "top" | "bottom";
  size: number;
  setSize: (size: number) => void;
  minSize: number;
  maxSize: number;
  children?: ReactNode;
}

export function Resizable({
  side,
  size,
  setSize,
  minSize,
  maxSize,
  children,
}: Readonly<ResizableProps>) {
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

    const controller = new AbortController();

    globalThis.addEventListener(
      "mousemove",
      (moveEvent) => {
        const delta = horizontal
          ? moveEvent.clientX - downEvent.clientX
          : moveEvent.clientY - downEvent.clientY;
        updateSize(delta);
      },
      { signal: controller.signal },
    );

    globalThis.addEventListener(
      "mouseup",
      () => {
        document.body.style.cursor = prevCursor;
        controller.abort();
      },
      { signal: controller.signal },
    );
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
    }
  }

  return (
    <div className={cn("flex", directionClasses[direction])}>
      <div
        className={horizontal ? "h-full" : "w-full"}
        style={horizontal ? { width: size } : { height: size }}
      >
        {children}
      </div>
      <div className="relative">
        <div
          tabIndex={0}
          onMouseDown={handleMouseDown}
          onKeyDown={handleKeyDown}
          role="separator"
          aria-label="Resize panel"
          aria-orientation={horizontal ? "horizontal" : "vertical"}
          aria-valuemin={minSize}
          aria-valuemax={maxSize}
          aria-valuenow={size}
          className={cn(
            "absolute focus-visible:outline-2 focus-visible:outline-(--accent-6)",
            horizontal
              ? "-left-px h-full w-0.5 cursor-ew-resize"
              : "-top-px h-0.5 w-full cursor-ns-resize",
          )}
        >
          <VisuallyHidden>Resize</VisuallyHidden>
        </div>
      </div>
    </div>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
