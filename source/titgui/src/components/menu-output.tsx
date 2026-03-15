/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { Box, Text } from "@radix-ui/themes";
import { useCallback, useEffect, useMemo, useRef } from "react";
import {
  TbClearAll as ClearIcon,
  TbDownload as DownloadIcon,
} from "react-icons/tb";

import { type MenuAction, useMenuAction } from "~/components/menu";
import { useSolver } from "~/components/solver";
import { downloadBlob } from "~/utils";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export function OutputMenu() {
  const { clearSolverOutput, solverOutput } = useSolver();

  const save = useCallback(() => {
    downloadBlob(
      "solver-output.txt",
      new Blob([solverOutput], { type: "text/plain" }),
    );
  }, [solverOutput]);

  const saveAction = useMemo<MenuAction>(
    () => ({
      name: "Save Output",
      icon: <DownloadIcon size={16} />,
      disabled: solverOutput.length === 0,
      onClick: save,
    }),
    [save, solverOutput.length],
  );

  useMenuAction(saveAction);

  const clearAction = useMemo<MenuAction>(
    () => ({
      name: "Clear Output",
      icon: <ClearIcon size={16} />,
      disabled: solverOutput.length === 0,
      onClick: clearSolverOutput,
    }),
    [clearSolverOutput, solverOutput.length],
  );

  useMenuAction(clearAction);

  const bottomRef = useRef<HTMLDivElement | null>(null);

  useEffect(() => {
    if (!solverOutput) return;
    bottomRef.current?.scrollIntoView({ behavior: "smooth" });
  }, [solverOutput]);

  return (
    <Box className="select-text hover:cursor-text">
      <Text size="1">
        <pre>{solverOutput}</pre>
      </Text>

      <div ref={bottomRef} />
    </Box>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
