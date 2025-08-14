/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { useMutation, useQuery, useQueryClient } from "@tanstack/react-query";
import { useCallback } from "react";

import {
  fetch_solver_config,
  save_solver_config,
} from "~/mock-solver-config-api";
import {
  type PropertyRecord,
  type RootSpec,
  type SolverConfig,
} from "~/solver-config";
import { isStateUpdater, type SetStateAction } from "~/utils";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

const queryKey = ["solver-config"];

export function useSolverConfig() {
  const queryClient = useQueryClient();

  const { data, isLoading } = useQuery({
    queryKey,
    queryFn: fetch_solver_config,
    staleTime: Number.POSITIVE_INFINITY,
  });

  const { mutate, isPending: isSaving } = useMutation({
    mutationFn: async (tree: PropertyRecord) => save_solver_config(tree),
    onMutate: async (nextTree) => {
      await queryClient.cancelQueries({ queryKey });
      const previous = queryClient.getQueryData<SolverConfig>(queryKey);
      if (previous) {
        queryClient.setQueryData<SolverConfig>(queryKey, {
          ...previous,
          tree: nextTree,
        });
      }
      return { previous };
    },
    onError: (_error, _nextTree, context) => {
      if (context?.previous) {
        queryClient.setQueryData(queryKey, context.previous);
      }
    },
  });

  const setTree = useCallback(
    (next: SetStateAction<PropertyRecord>) => {
      const previous = queryClient.getQueryData<SolverConfig>(queryKey);
      if (!previous) return;

      const nextTree = isStateUpdater(next) ? next(previous.tree) : next;
      mutate(nextTree);
    },
    [mutate, queryClient],
  );

  return [
    isLoading,
    isSaving,
    data?.spec ?? null,
    data?.tree ?? null,
    setTree,
  ] as const satisfies readonly [
    boolean,
    boolean,
    RootSpec | null,
    PropertyRecord | null,
    (next: SetStateAction<PropertyRecord>) => void,
  ];
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
