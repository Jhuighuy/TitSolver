/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { useMutation, useQuery, useQueryClient } from "@tanstack/react-query";
import { useCallback, useEffect, useMemo } from "react";
import type { ZodType } from "zod";

import { ipc } from "~/renderer/common/ipc";
import { logger } from "~/renderer/common/logging";
import { applyStateUpdate, type SetStateAction } from "~/renderer/common/utils";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export function useWindowState<T>(
  key: string,
  schema: ZodType<T>,
  fallbackValue: T,
) {
  const queryClient = useQueryClient();
  const queryKey = useMemo(() => ["window", "persist", key], [key]);

  const { data: value } = useQuery({
    queryKey,
    queryFn: async () => {
      const rawValue = await ipc.window.persistGet(key);
      if (rawValue === undefined) return fallbackValue;

      const { success, data: value, error } = schema.safeParse(rawValue);
      if (!success) {
        logger.warn(
          `Invalid persisted value for '${key}', reverting to fallback.\n`,
          `Error: '${error.message}'.`,
        );
        mutate(fallbackValue);
        return fallbackValue;
      }

      return value;
    },
    staleTime: Number.POSITIVE_INFINITY,
    gcTime: Number.POSITIVE_INFINITY,
  });

  const { mutate } = useMutation({
    mutationFn: async (value: T) => {
      await ipc.window.persistSet(key, schema.parse(value));
    },
    onMutate: async (next) => {
      await queryClient.cancelQueries({ queryKey });
      const previous = queryClient.getQueryData<T>(queryKey);
      queryClient.setQueryData(queryKey, next);
      return { previous };
    },
    onError: (error, _next, context) => {
      // Roll the optimistic update back and surface the failure.
      queryClient.setQueryData(queryKey, context?.previous);
      logger.err(`Failed to persist '${key}'.\n`, error);
    },
  });

  const setValue = useCallback(
    (next: SetStateAction<T>) => {
      const prev = queryClient.getQueryData<T>(queryKey) ?? fallbackValue;
      mutate(applyStateUpdate(prev, next));
    },
    [queryClient, queryKey, mutate, fallbackValue],
  );

  return [value ?? fallbackValue, setValue] as const;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export function useWindowIsFullScreen() {
  const queryClient = useQueryClient();
  const queryKey = useMemo(() => ["window", "is-full-screen"], []);

  const { data: isFullScreen } = useQuery({
    queryKey,
    queryFn: async () => ipc.window.isFullScreen(),
    staleTime: Number.POSITIVE_INFINITY,
    gcTime: Number.POSITIVE_INFINITY,
  });

  useEffect(() => {
    return ipc.window.onFullScreenChanged((isFullScreen) => {
      queryClient.setQueryData(queryKey, isFullScreen);
    });
  }, [queryClient, queryKey]);

  return isFullScreen ?? false;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
