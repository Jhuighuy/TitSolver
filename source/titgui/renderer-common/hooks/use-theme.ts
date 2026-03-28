/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { useMutation, useQuery, useQueryClient } from "@tanstack/react-query";
import { useCallback, useMemo } from "react";

import { applyStateUpdate, type SetStateAction } from "~/renderer-common/utils";
import type { Theme } from "~/shared/theme";
import { assert } from "~/shared/utils";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export function useThemeState() {
  const queryClient = useQueryClient();
  const queryKey = useMemo(() => ["window", "theme"], []);

  const { data: theme } = useQuery({
    queryKey,
    queryFn: async () => await globalThis.theme?.get(),
    staleTime: Number.POSITIVE_INFINITY,
    gcTime: Number.POSITIVE_INFINITY,
  });

  const { mutate } = useMutation({
    mutationFn: async (nextTheme: Theme) => {
      assert(globalThis.theme !== undefined);
      await globalThis.theme.set(nextTheme);
    },
    onMutate: async (nextTheme) => {
      await queryClient.cancelQueries({ queryKey });
      queryClient.setQueryData(queryKey, nextTheme);
    },
  });

  const setTheme = useCallback(
    (next: SetStateAction<Theme>) => {
      const prev = queryClient.getQueryData<Theme>(queryKey) ?? "system";
      mutate(applyStateUpdate(prev, next));
    },
    [mutate, queryClient, queryKey],
  );

  return [theme ?? "system", setTheme] as const;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
