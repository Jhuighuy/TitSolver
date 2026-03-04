/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { useMutation, useQuery, useQueryClient } from "@tanstack/react-query";
import {
  createContext,
  type ReactNode,
  useEffect,
  useContext,
  useRef,
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

const solverStatusQueryKey = ["solver", "status"] as const;
const storageNumFramesQueryKey = ["storage", "num-frames"] as const;

export function SolverProvider({ children }: Readonly<SolverProviderProps>) {
  const queryClient = useQueryClient();
  const prevOutputSizeRef = useRef(0);

  const statusQuery = useQuery({
    queryKey: solverStatusQueryKey,
    queryFn: getSolverStatus,
    refetchInterval(query) {
      const data = query.state.data;
      return data?.isRunning === true ? 200 : false;
    },
  });

  const runMutation = useMutation({
    mutationFn: runSolverRequest,
    onSuccess: () => {
      void queryClient.invalidateQueries({ queryKey: solverStatusQueryKey });
    },
  });

  const stopMutation = useMutation({
    mutationFn: stopSolverRequest,
    onSuccess: () => {
      void queryClient.invalidateQueries({ queryKey: solverStatusQueryKey });
    },
  });

  useEffect(() => {
    const output = statusQuery.data?.output;
    if (output === undefined) return;

    const prevOutputSize = prevOutputSizeRef.current;
    const outputChanged = prevOutputSize !== output.length;
    prevOutputSizeRef.current = output.length;
    if (!outputChanged) return;

    void queryClient.invalidateQueries({
      queryKey: storageNumFramesQueryKey,
    });
  }, [queryClient, statusQuery.data?.output]);

  const solver: Solver = {
    isSolverRunning: statusQuery.data?.isRunning ?? false,
    solverOutput: statusQuery.data?.output ?? "",
    runSolver() {
      if (runMutation.isPending || stopMutation.isPending) return;
      runMutation.mutate();
    },
    stopSolver() {
      if (runMutation.isPending || stopMutation.isPending) return;
      stopMutation.mutate();
    },
  };

  return (
    <SolverContext.Provider value={solver}>{children}</SolverContext.Provider>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
