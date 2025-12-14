/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import {
  createContext,
  type ReactNode,
  useCallback,
  useContext,
  useMemo,
  useRef,
  useState,
} from "react";
import { z } from "zod";

import { useConnection } from "~/components/connection";
import { assert } from "~/utils";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export type Solver = {
  isSolverRunning: boolean;
  solverOutput: string;
  runSolver: () => void;
  stopSolver: () => void;
};

const SolverContext = createContext<Solver | null>(null);

export function useSolver(): Solver {
  const context = useContext(SolverContext);
  assert(context !== null, "Solver is not available.");
  return context;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

type SolverProviderProps = {
  children: ReactNode;
};

export function SolverProvider({ children }: Readonly<SolverProviderProps>) {
  const { sendMessage } = useConnection();

  const isSolverRunningRef = useRef(false);
  const [isSolverRunning, setIsSolverRunning] = useState(false);
  const [solverOutput, setSolverOutput] = useState("");

  const runSolver = useCallback(() => {
    assert(!isSolverRunningRef.current);
    isSolverRunningRef.current = true;
    setIsSolverRunning(true);

    setSolverOutput("");

    sendMessage({ type: "run" }, (responseRaw) => {
      const response = runSchema.parse(responseRaw);
      switch (response.kind) {
        case "stdout":
        case "stderr":
          setSolverOutput((prev) => prev + response.data);
          break;

        case "exit": {
          const { code, signal } = response;
          setSolverOutput(
            (prev) =>
              `${prev}\n[Process exited with code ${code}, signal ${signal}]\n`
          );

          isSolverRunningRef.current = false;
          setIsSolverRunning(false);
          break;
        }

        default:
          assert(false);
      }
    });
  }, [sendMessage]);

  const stopSolver = useCallback(() => {
    assert(isSolverRunningRef.current);

    sendMessage({ type: "stop" });
  }, [sendMessage]);

  const solver = useMemo<Solver>(
    () => ({
      isSolverRunning,
      solverOutput,
      runSolver,
      stopSolver,
    }),
    [isSolverRunning, solverOutput, runSolver, stopSolver]
  );

  return (
    <SolverContext.Provider value={solver}>{children}</SolverContext.Provider>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

const runSchema = z.union([
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
