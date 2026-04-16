/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { useMutation, useQuery, useQueryClient } from "@tanstack/react-query";
import { useCallback, useMemo } from "react";

import { applyStateUpdate, type SetStateAction } from "~/renderer/common/utils";
import {
  propertyDocumentSchema,
  type PropertyDocument,
  type Tree,
} from "~/shared/properties";
import { assert } from "~/shared/utils";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export function useProperties() {
  const queryClient = useQueryClient();
  const queryKey = useMemo(() => ["properties", "document"], []);

  const { data: document } = useQuery({
    queryKey,
    queryFn: async () => {
      assert(globalThis.properties !== undefined);
      return propertyDocumentSchema.parse(
        await globalThis.properties.getDocument(),
      );
    },
    gcTime: Number.POSITIVE_INFINITY,
    staleTime: Number.POSITIVE_INFINITY,
  });

  const { mutate } = useMutation({
    mutationFn: async ({
      tree,
      revision,
    }: {
      tree: Tree;
      revision: number;
    }) => {
      assert(globalThis.properties !== undefined);
      return propertyDocumentSchema.parse(
        await globalThis.properties.updateTree(tree, revision),
      );
    },
    onSuccess: (nextDocument) => {
      queryClient.setQueryData<PropertyDocument>(queryKey, nextDocument);
    },
  });

  const setTree = useCallback(
    (next: SetStateAction<Tree>) => {
      if (document === undefined) return;

      const updated = applyStateUpdate(document.tree, next);
      mutate({ tree: updated, revision: document.revision });
    },
    [document, mutate],
  );

  const isComplete = document?.issues.length === 0;

  return {
    spec: document?.spec,
    tree: document?.tree,
    issues: document?.issues ?? [],
    namespaceTable: document?.namespaceTable ?? {},
    isComplete,
    isLoading: document === undefined,
    setTree,
  } as const;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
