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
import { z } from "zod";

import { useConnection } from "~/components/connection";
import { assert } from "~/utils";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export type Solver = {
  isSolverRunning: boolean;
  solverOutput: string;
  elapsedMs: number;
  samples: SolverSample[];
  runSolver: () => void;
  stopSolver: () => void;
  clearSolverOutput: () => void;
};

export type SolverSample = {
  timestamp: number;
  cpuPercent: number;
  memoryBytes: number;
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
  const [elapsedMs, setElapsedMs] = useState(0);
  const [runStartedAt, setRunStartedAt] = useState<number | null>(null);
  const [samples, setSamples] = useState<SolverSample[]>([]);

  useEffect(() => {
    if (!isSolverRunning || runStartedAt === null) return;

    const intervalID = window.setInterval(() => {
      setElapsedMs(Date.now() - runStartedAt);
    }, 1000);

    return () => window.clearInterval(intervalID);
  }, [isSolverRunning, runStartedAt]);

  const runSolver = useCallback(() => {
    assert(!isSolverRunningRef.current);
    isSolverRunningRef.current = true;
    setIsSolverRunning(true);

    const startedAt = Date.now();
    setRunStartedAt(startedAt);
    setElapsedMs(0);
    setSolverOutput("");
    setSamples([]);

    sendMessage({ type: "run" }, (responseRaw) => {
      const response = runSchema.parse(responseRaw);
      switch (response.kind) {
        case "stdout":
        case "stderr":
          setSolverOutput((prev) => prev + response.data);
          if (
            response.timestamp !== undefined &&
            response.cpuPercent !== undefined &&
            response.memoryBytes !== undefined
          ) {
            setSamples((prev) => {
              const sample = {
                timestamp: response.timestamp,
                cpuPercent: response.cpuPercent,
                memoryBytes: response.memoryBytes,
              };
              return prev.at(-1)?.timestamp === sample.timestamp
                ? prev
                : [...prev, sample];
            });
          }
          break;

        case "exit": {
          const { code, signal } = response;
          setSolverOutput(
            (prev) =>
              `${prev}\n[Process exited with code ${code}, signal ${signal}]\n`,
          );

          isSolverRunningRef.current = false;
          setIsSolverRunning(false);
          setElapsedMs(Date.now() - startedAt);
          setRunStartedAt(null);
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

  const clearSolverOutput = useCallback(() => {
    setSolverOutput("");
  }, []);

  const solver = useMemo<Solver>(
    () => ({
      isSolverRunning,
      solverOutput,
      elapsedMs,
      samples,
      runSolver,
      stopSolver,
      clearSolverOutput,
    }),
    [
      clearSolverOutput,
      elapsedMs,
      isSolverRunning,
      samples,
      solverOutput,
      runSolver,
      stopSolver,
    ],
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
    timestamp: z.number().optional(),
    cpuPercent: z.number().optional(),
    memoryBytes: z.number().optional(),
  }),
  z.object({
    kind: z.literal("stderr"),
    data: z.string(),
    timestamp: z.number().optional(),
    cpuPercent: z.number().optional(),
    memoryBytes: z.number().optional(),
  }),
  z.object({
    kind: z.literal("exit"),
    code: z.number(),
    signal: z.number(),
  }),
]);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
