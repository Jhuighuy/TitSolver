/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { Box, Text } from "@radix-ui/themes";
import { useEffect, useMemo, useRef } from "react";
import { TbRun as RunIcon, TbHandStop as StopIcon } from "react-icons/tb";

import { type MenuAction, useMenuAction } from "~/components/menu";
import { useSolver } from "~/components/solver";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export function RunMenu() {
  // ---- Solver. --------------------------------------------------------------

  const { isSolverRunning, solverOutput, runSolver, stopSolver } = useSolver();

  const runAction = useMemo<MenuAction>(
    () => ({
      name: "Run Solver",
      icon: <RunIcon size={16} />,
      disabled: isSolverRunning,
      onClick: runSolver,
    }),
    [isSolverRunning, runSolver]
  );

  useMenuAction(runAction);

  const stopAction = useMemo<MenuAction>(
    () => ({
      name: "Stop Solver",
      icon: <StopIcon size={16} />,
      disabled: !isSolverRunning,
      onClick: stopSolver,
    }),
    [isSolverRunning, stopSolver]
  );

  useMenuAction(stopAction);

  // ---- Layout. --------------------------------------------------------------

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
