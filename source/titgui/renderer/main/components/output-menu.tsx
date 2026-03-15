/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { IconClearAll, IconDownload } from "@tabler/icons-react";
import { useVirtualizer } from "@tanstack/react-virtual";
import { useEffect, useMemo, useRef } from "react";

import { Box } from "~/renderer/common/components/layout";
import {
  type MenuAction,
  useMenuAction,
} from "~/renderer/common/components/menu";
import { ScrollArea } from "~/renderer/common/components/scroll-area";
import { Text } from "~/renderer/common/components/text";
import { useSolver } from "~/renderer/main/hooks/use-solver";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export function OutputMenu() {
  // ---- Solver. --------------------------------------------------------------

  const { clearSolverOutput, downloadSolverOutput, outputLines } = useSolver();

  const saveAction = useMemo<MenuAction>(
    () => ({
      name: "Save Output",
      icon: <IconDownload />,
      disabled: outputLines.length === 0,
      onClick: downloadSolverOutput,
    }),
    [downloadSolverOutput, outputLines.length],
  );

  useMenuAction(saveAction);

  const clearAction = useMemo<MenuAction>(
    () => ({
      name: "Clear Output",
      icon: <IconClearAll />,
      disabled: outputLines.length === 0,
      onClick: clearSolverOutput,
    }),
    [clearSolverOutput, outputLines.length],
  );

  useMenuAction(clearAction);

  // ---- Layout. --------------------------------------------------------------

  const scrollViewportRef = useRef<HTMLDivElement>(null);

  // eslint-disable-next-line react-hooks/incompatible-library
  const rowVirtualizer = useVirtualizer({
    count: outputLines.length,
    getScrollElement: () => scrollViewportRef.current,
    estimateSize: () => 12,
    overscan: 24,
  });

  useEffect(() => {
    if (outputLines.length === 0) return;

    const scrollViewport = scrollViewportRef.current;
    if (scrollViewport === null) return;
    scrollViewport.scrollTop = scrollViewport.scrollHeight;
  }, [outputLines.length, scrollViewportRef]);

  return (
    <ScrollArea viewportRef={scrollViewportRef}>
      <Box size="100%" p="1" className="select-text">
        <Box
          position="relative"
          width="100%"
          height={`${rowVirtualizer.getTotalSize()}px`}
          className="select-text"
        >
          {rowVirtualizer.getVirtualItems().map((virtualRow) => {
            const line = outputLines[virtualRow.index];
            return (
              <Text
                key={line.id}
                data-index={virtualRow.index}
                as="pre"
                size="1"
                mono
                color={line.stream === "stderr" ? "red" : "default"}
                className="absolute top-0 left-0 m-0 w-full whitespace-pre"
                style={{ transform: `translateY(${virtualRow.start}px)` }}
                ref={rowVirtualizer.measureElement}
              >
                {line.text}
              </Text>
            );
          })}
        </Box>
      </Box>
    </ScrollArea>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
