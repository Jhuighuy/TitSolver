/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { useMutation, useQuery, useQueryClient } from "@tanstack/react-query";
import {
  createContext,
  type ReactNode,
  useContext,
  useEffect,
  useMemo,
  useRef,
  useState,
  useSyncExternalStore,
} from "react";

import {
  getSolverStatus,
  runSolver as runSolverRequest,
  stopSolver as stopSolverRequest,
} from "~/backend-api";
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

class SolverSamplesStore {
  private readonly listeners = new Set<() => void>();
  private samples: SolverSample[] = [];
  private lastTimestamp: number | null = null;

  public subscribe = (listener: () => void) => {
    this.listeners.add(listener);
    return () => this.listeners.delete(listener);
  };

  public getSnapshot = () => this.samples;

  public clear() {
    this.samples = [];
    this.lastTimestamp = null;
    this.emit();
  }

  public append(sample: SolverSample) {
    if (this.lastTimestamp === sample.timestamp) return;
    this.lastTimestamp = sample.timestamp;
    this.samples = [...this.samples, sample];
    this.emit();
  }

  private emit() {
    for (const listener of this.listeners) listener();
  }
}

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

const solverStatusQueryKey = ["solver", "status"] as const;
const storageNumFramesQueryKey = ["storage", "num-frames"] as const;

export function SolverProvider({ children }: Readonly<SolverProviderProps>) {
  const queryClient = useQueryClient();
  const [samplesStore] = useState(() => new SolverSamplesStore());
  const previousOutputSizeRef = useRef(0);

  const [elapsedMs, setElapsedMs] = useState(0);
  const [runStartedAt, setRunStartedAt] = useState<number | null>(null);
  const [clearedOutputLength, setClearedOutputLength] = useState(0);
  const samples = useSyncExternalStore(
    samplesStore.subscribe,
    samplesStore.getSnapshot,
  );

  const statusQuery = useQuery({
    queryKey: solverStatusQueryKey,
    queryFn: getSolverStatus,
    refetchInterval(query) {
      const isRunning = query.state.data?.isRunning ?? false;
      return isRunning ? 200 : false;
    },
  });

  const runMutation = useMutation({
    mutationFn: runSolverRequest,
    onSuccess: () => {
      const startedAt = Date.now();
      previousOutputSizeRef.current = 0;
      samplesStore.clear();
      setRunStartedAt(startedAt);
      setElapsedMs(0);
      setClearedOutputLength(0);
      void queryClient.invalidateQueries({ queryKey: solverStatusQueryKey });
    },
  });

  const stopMutation = useMutation({
    mutationFn: stopSolverRequest,
    onSuccess: () => {
      setRunStartedAt(null);
      void queryClient.invalidateQueries({ queryKey: solverStatusQueryKey });
    },
  });

  useEffect(() => {
    if (runStartedAt === null) return;
    if (!(statusQuery.data?.isRunning ?? false)) return;

    const intervalID = window.setInterval(() => {
      setElapsedMs(Date.now() - runStartedAt);
    }, 1000);

    return () => window.clearInterval(intervalID);
  }, [runStartedAt, statusQuery.data?.isRunning]);

  useEffect(() => {
    const status = statusQuery.data;
    if (status === undefined) return;

    const outputChanged =
      previousOutputSizeRef.current !== status.output.length;
    previousOutputSizeRef.current = status.output.length;

    if (outputChanged) {
      void queryClient.invalidateQueries({
        queryKey: storageNumFramesQueryKey,
      });
    }

    if (
      status.timestamp !== undefined &&
      status.cpuPercent !== undefined &&
      status.memoryBytes !== undefined
    ) {
      samplesStore.append({
        timestamp: status.timestamp,
        cpuPercent: status.cpuPercent,
        memoryBytes: status.memoryBytes,
      });
    }
  }, [queryClient, samplesStore, statusQuery.data]);

  const fullOutput = statusQuery.data?.output ?? "";
  const solverOutput = fullOutput.slice(
    Math.min(clearedOutputLength, fullOutput.length),
  );
  const isSolverRunning = statusQuery.data?.isRunning ?? runMutation.isPending;

  const solver = useMemo<Solver>(
    () => ({
      isSolverRunning,
      solverOutput,
      elapsedMs,
      samples,
      runSolver() {
        if (runMutation.isPending || stopMutation.isPending) return;
        runMutation.mutate();
      },
      stopSolver() {
        if (runMutation.isPending || stopMutation.isPending) return;
        stopMutation.mutate();
      },
      clearSolverOutput() {
        setClearedOutputLength(fullOutput.length);
      },
    }),
    [
      elapsedMs,
      fullOutput.length,
      isSolverRunning,
      runMutation,
      samples,
      solverOutput,
      stopMutation,
    ],
  );

  return (
    <SolverContext.Provider value={solver}>{children}</SolverContext.Provider>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
