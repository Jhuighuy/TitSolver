/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { useMutation, useQuery, useQueryClient } from "@tanstack/react-query";
import { useCallback, useMemo } from "react";
import type { ZodType } from "zod";

import { logger } from "~/logging";
import { isStateUpdater, type SetStateAction } from "~/utils";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export function usePersistedState<T>(
  key: string,
  schema: ZodType<T>,
  fallbackValue: T,
) {
  const queryClient = useQueryClient();
  const queryKey = useMemo(() => ["persist", key], [key]);

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  const { data } = useQuery({
    queryKey,
    queryFn: async () => {
      if (window.persistedState === undefined) return fallbackValue;

      const persistedValue = await window.persistedState.get(key);
      if (persistedValue === undefined) return fallbackValue;

      const parsed = schema.safeParse(persistedValue);
      if (!parsed.success) {
        logger.warn(
          `Invalid persisted value for '${key}', using fallback.\n`,
          `Error: '${parsed.error.message}'`,
        );
        mutate(fallbackValue);
        return fallbackValue;
      }

      return parsed.data;
    },
    staleTime: Number.POSITIVE_INFINITY,
    gcTime: Number.POSITIVE_INFINITY,
  });

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  const { mutate } = useMutation({
    mutationFn: async (value: T) => {
      await window.persistedState?.set(key, schema.parse(value));
    },
    onMutate: async (next) => {
      await queryClient.cancelQueries({ queryKey });
      const previous = queryClient.getQueryData<T>(queryKey);
      queryClient.setQueryData(queryKey, next);
      return { previous };
    },
    onError: (_error, _next, context) => {
      queryClient.setQueryData(queryKey, context?.previous ?? fallbackValue);
    },
  });

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  const setValue = useCallback(
    (next: SetStateAction<T>) => {
      const previous = queryClient.getQueryData<T>(queryKey) ?? fallbackValue;
      const nextValue = isStateUpdater(next) ? next(previous) : next;
      mutate(nextValue);
    },
    [fallbackValue, mutate, queryClient, queryKey],
  );

  return [data ?? fallbackValue, setValue] as const;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
