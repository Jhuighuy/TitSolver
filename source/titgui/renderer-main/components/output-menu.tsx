/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { IconClearAll, IconDownload } from "@tabler/icons-react";
import { useEffect, useMemo, useRef } from "react";
import { Box } from "~/renderer-common/components/layout";
import { Text } from "~/renderer-common/components/text";

import {
  type MenuAction,
  useMenuAction,
} from "~/renderer-common/components/menu";
import { useSolver } from "~/renderer-main/hooks/use-solver";

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

  const bottomRef = useRef<HTMLDivElement | null>(null);

  useEffect(() => {
    if (outputLines.length === 0) return;
    bottomRef.current?.scrollIntoView();
  }, [outputLines.length]);

  return (
    <Box size="100%" p="1" className="select-text">
      {outputLines.map((line) => (
        <Text
          key={line.id}
          as="pre"
          size="1"
          mono
          color={line.stream === "stderr" ? "red" : "default"}
        >
          {line.text}
        </Text>
      ))}

      <div ref={bottomRef} />
    </Box>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
