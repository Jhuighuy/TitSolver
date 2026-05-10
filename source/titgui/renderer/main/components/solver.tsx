/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import {
  createContext,
  type ReactNode,
  useCallback,
  useContext,
  useEffect,
  useMemo,
  useRef,
  useState,
} from "react";

import { solverEventSchema } from "~/shared/solver";
import { assert } from "~/shared/utils";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export interface Solver {
  isSolverRunning: boolean;
  solverOutput: string;
  runSolver: () => void;
  stopSolver: () => void;
}

const SolverContext = createContext<Solver | null>(null);

export function useSolver(): Solver {
  const context = useContext(SolverContext);
  assert(context !== null, "Solver is not available.");
  return context;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

interface SolverProviderProps {
  children: ReactNode;
}

export function SolverProvider({ children }: Readonly<SolverProviderProps>) {
  const isSolverRunningRef = useRef(false);
  const [isSolverRunning, setIsSolverRunning] = useState(false);
  const [solverOutput, setSolverOutput] = useState("");

  useEffect(() => {
    void globalThis.session?.isSolverRunning().then((running) => {
      isSolverRunningRef.current = running;
      setIsSolverRunning(running);
    });

    return globalThis.session?.onSolverEvent((responseRaw) => {
      const response = solverEventSchema.parse(responseRaw);
      switch (response.kind) {
        case "stdout":
        case "stderr":
          setSolverOutput((prev) => prev + response.data);
          break;

        case "exit":
          setSolverOutput(
            (prev) =>
              `${prev}\n[Process exited with code ${response.code}, signal ${response.signal}]\n`,
          );
          isSolverRunningRef.current = false;
          setIsSolverRunning(false);
          break;
      }
    });
  }, []);

  const runSolver = useCallback(() => {
    assert(!isSolverRunningRef.current);
    isSolverRunningRef.current = true;
    setIsSolverRunning(true);

    setSolverOutput("");
    void globalThis.session?.runSolver();
  }, []);

  const stopSolver = useCallback(() => {
    assert(isSolverRunningRef.current);
    void globalThis.session?.stopSolver();
  }, []);

  const solver = useMemo<Solver>(
    () => ({
      isSolverRunning,
      solverOutput,
      runSolver,
      stopSolver,
    }),
    [isSolverRunning, solverOutput, runSolver, stopSolver],
  );

  return (
    <SolverContext.Provider value={solver}>{children}</SolverContext.Provider>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
