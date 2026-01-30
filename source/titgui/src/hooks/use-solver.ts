/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { useCallback } from "react";
import { create } from "zustand";
import { z } from "zod";

import { useConnection } from "~/hooks/use-connection";
import { assert } from "~/utils";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export type Solver = {
  isSolverRunning: boolean;
  solverOutput: string;
  runSolver: () => void;
  stopSolver: () => void;
};

export function useSolver(): Solver {
  const { sendMessage } = useConnection();
  const {
    isSolverRunning,
    solverOutput,
    runSolver: runSolverBase,
    stopSolver: stopSolverBase,
    appendOutput,
  } = useSolverStore();

  const runSolver = useCallback(() => {
    runSolverBase();
    sendMessage({ type: "run" }, (responseRaw) => {
      if (responseRaw instanceof Error) {
        appendOutput(`\n[Exited with error ${responseRaw.message}.]\n`);
        stopSolverBase();
        return;
      }

      const response = responeSchema.parse(responseRaw);
      switch (response.kind) {
        case "stdout":
        case "stderr": {
          appendOutput(response.data);
          break;
        }
        case "exit": {
          const { code, signal } = response;
          appendOutput(`\n[Exited with code ${code}, signal ${signal}.]\n`);
          stopSolverBase();
          break;
        }
        default:
          assert(false);
      }
    });
  }, [sendMessage, runSolverBase, stopSolverBase, appendOutput]);

  const stopSolver = useCallback(() => {
    sendMessage({ type: "stop" });
    stopSolverBase();
  }, [sendMessage, stopSolverBase]);

  return {
    isSolverRunning,
    solverOutput,
    runSolver,
    stopSolver,
  };
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

type SolverState = {
  isSolverRunning: boolean;
  solverOutput: string;
  runSolver: () => void;
  stopSolver: () => void;
  appendOutput: (data: string) => void;
};

const useSolverStore = create<SolverState>((set, get) => ({
  isSolverRunning: false,
  solverOutput: "",
  runSolver: () => {
    assert(!get().isSolverRunning);
    set({ isSolverRunning: true, solverOutput: "" });
  },
  stopSolver: () => {
    assert(get().isSolverRunning);
    set({ isSolverRunning: false });
  },
  appendOutput: (data) => {
    // Note: No need to assert `isSolverRunning` here, as output may arrive
    //       after the solver stop request.
    set((state) => ({ solverOutput: state.solverOutput + data }));
  },
}));

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

const responeSchema = z.union([
  z.object({
    kind: z.literal("stdout"),
    data: z.string(),
  }),
  z.object({
    kind: z.literal("stderr"),
    data: z.string(),
  }),
  z.object({
    kind: z.literal("exit"),
    code: z.number(),
    signal: z.number(),
  }),
]);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
