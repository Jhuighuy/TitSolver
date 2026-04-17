/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { useMutation, useQuery, useQueryClient } from "@tanstack/react-query";
import { useCallback, useEffect, useMemo } from "react";
import { applyStateUpdate, SetStateAction } from "~/renderer-common/utils";

import { fetchSpec, fetchTree, postTree } from "~/shared/props-mock";
import { specSchema } from "~/shared/spec";
import {
  normalizeTree,
  treeEquals,
  treeSchema,
  type Tree,
} from "~/shared/tree";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export function useProperties() {
  const queryClient = useQueryClient();
  const querySpecKey = useMemo(() => ["properties", "spec"], []);
  const queryTreeKey = useMemo(() => ["properties", "tree"], []);

  const { data: spec } = useQuery({
    queryKey: querySpecKey,
    queryFn: async () => specSchema.parse(await fetchSpec()),
    gcTime: Number.POSITIVE_INFINITY,
    staleTime: Number.POSITIVE_INFINITY,
  });

  const { data: tree } = useQuery({
    queryKey: queryTreeKey,
    queryFn: async () => treeSchema.parse(await fetchTree()),
    gcTime: Number.POSITIVE_INFINITY,
    staleTime: Number.POSITIVE_INFINITY,
  });

  const { mutate } = useMutation({
    mutationFn: postTree,
    onMutate: async (next) => {
      await queryClient.cancelQueries({ queryKey: queryTreeKey });
      const previous = queryClient.getQueryData<Tree>(queryTreeKey);
      queryClient.setQueryData(queryTreeKey, next);
      return { previous };
    },
  });

  // Normalize tree.
  const normalized = useMemo(() => {
    if (spec === undefined || tree === undefined) return null;
    return normalizeTree(spec, tree);
  }, [spec, tree]);

  useEffect(() => {
    if (
      tree === undefined ||
      normalized === null ||
      treeEquals(tree, normalized.tree)
    ) {
      return;
    }

    mutate(normalized.tree);
  }, [mutate, normalized, tree]);

  const setTree = useCallback(
    (next: SetStateAction<Tree>) => {
      if (spec === undefined) return;

      const previous = normalized === null ? null : normalized.tree;
      const updated = applyStateUpdate(previous, next);
      mutate(normalizeTree(spec, updated).tree);
    },
    [mutate, normalized, spec],
  );

  return {
    spec,
    tree: normalized?.tree,
    isComplete: normalized?.isComplete ?? false,
    warnings: normalized?.warnings ?? [],
    isLoading: spec === undefined || tree === undefined,
    setTree,
  } as const;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
