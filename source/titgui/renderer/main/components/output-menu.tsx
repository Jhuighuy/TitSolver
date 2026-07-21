/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { IconClearAll, IconDownload } from "@tabler/icons-react";
import { useVirtualizer } from "@tanstack/react-virtual";
import { useAtomValue } from "jotai";
import { useEffect, useMemo, useRef } from "react";

import {
  type MenuAction,
  useMenuAction,
} from "~/renderer/common/components/menu";
import { ScrollArea } from "~/renderer/common/components/scroll-area";
import { Text } from "~/renderer/common/components/text";
import {
  clearSolverOutput,
  downloadSolverOutput,
  solverOutputLinesAtom,
} from "~/renderer/main/state/solver";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Follow the output only while the reader is already at the bottom.
const AUTOSCROLL_TOLERANCE_PX = 48;

export function OutputMenu() {
  "use no memo"; // The virtualizer is incompatible with the React Compiler.

  const outputLines = useAtomValue(solverOutputLinesAtom);

  // ---- Actions. -------------------------------------------------------------

  const saveAction = useMemo<MenuAction>(
    () => ({
      name: "Save Output",
      icon: <IconDownload />,
      disabled: outputLines.length === 0,
      onClick: downloadSolverOutput,
    }),
    [outputLines.length],
  );

  useMenuAction(saveAction);

  const clearAction = useMemo<MenuAction>(
    () => ({
      name: "Clear Output",
      icon: <IconClearAll />,
      disabled: outputLines.length === 0,
      onClick: clearSolverOutput,
    }),
    [outputLines.length],
  );

  useMenuAction(clearAction);

  // ---- Layout. --------------------------------------------------------------

  const scrollViewportRef = useRef<HTMLDivElement>(null);

  const rowVirtualizer = useVirtualizer({
    count: outputLines.length,
    getScrollElement: () => scrollViewportRef.current,
    estimateSize: () => 14,
    // The log is bounded, so appends shift the indices of all lines; key the
    // measurements by the stable line IDs, or heights get attributed to the
    // wrong lines and rows render on top of each other.
    getItemKey: (index) => outputLines[index].id,
    overscan: 24,
  });

  useEffect(() => {
    const viewport = scrollViewportRef.current;
    if (viewport === null || outputLines.length === 0) return;

    const distanceFromBottom =
      viewport.scrollHeight - viewport.scrollTop - viewport.clientHeight;
    if (distanceFromBottom < AUTOSCROLL_TOLERANCE_PX) {
      viewport.scrollTop = viewport.scrollHeight;
    }
  }, [outputLines]);

  return (
    <ScrollArea viewportRef={scrollViewportRef}>
      <div className="size-full p-1 select-text">
        <div
          className="relative w-full"
          style={{ height: rowVirtualizer.getTotalSize() }}
        >
          {rowVirtualizer.getVirtualItems().map((virtualRow) => {
            const line = outputLines[virtualRow.index];
            return (
              <Text
                key={virtualRow.key}
                data-index={virtualRow.index}
                as="pre"
                mono
                color={line.stream === "stderr" ? "danger" : "default"}
                // Empty lines must keep their line height, or they measure as
                // zero-height elements.
                className="absolute top-0 left-0 m-0 min-h-(--leading-1) w-full whitespace-pre"
                style={{ transform: `translateY(${virtualRow.start}px)` }}
                ref={rowVirtualizer.measureElement}
              >
                {line.text}
              </Text>
            );
          })}
        </div>
      </div>
    </ScrollArea>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
