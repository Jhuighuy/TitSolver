/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { IconHandStop, IconRun } from "@tabler/icons-react";
import { useAtomValue } from "jotai";
import { useEffect, useMemo, useRef } from "react";

import {
  type MenuAction,
  useMenuAction,
} from "~/renderer/common/components/menu";
import { Text } from "~/renderer/common/components/text";
import {
  isSolverRunningAtom,
  runSolver,
  solverOutputAtom,
  stopSolver,
} from "~/renderer/main/state/solver";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export function DashboardMenu() {
  // ---- Solver. --------------------------------------------------------------

  const isSolverRunning = useAtomValue(isSolverRunningAtom);
  const solverOutput = useAtomValue(solverOutputAtom);

  const runAction = useMemo<MenuAction>(
    () => ({
      name: "Run Solver",
      icon: <IconRun />,
      disabled: isSolverRunning,
      onClick: runSolver,
    }),
    [isSolverRunning],
  );

  useMenuAction(runAction);

  const stopAction = useMemo<MenuAction>(
    () => ({
      name: "Stop Solver",
      icon: <IconHandStop />,
      disabled: !isSolverRunning,
      onClick: stopSolver,
    }),
    [isSolverRunning],
  );

  useMenuAction(stopAction);

  // ---- Layout. --------------------------------------------------------------

  const bottomRef = useRef<HTMLDivElement | null>(null);

  useEffect(() => {
    if (!solverOutput) return;
    bottomRef.current?.scrollIntoView({ behavior: "smooth" });
  }, [solverOutput]);

  return (
    <div className="select-text hover:cursor-text">
      <Text size="1">
        <pre>{solverOutput}</pre>
      </Text>

      <div ref={bottomRef} />
    </div>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
