/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { useMutation, useQuery, useQueryClient } from "@tanstack/react-query";
import { useCallback, useEffect, useMemo } from "react";
import { type ZodType } from "zod";

import { logger } from "~/renderer-common/logging";
import { applyStateUpdate, type SetStateAction } from "~/renderer-common/utils";
import { assert } from "~/shared/utils";

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
      const rawValue = await globalThis.windowState?.persistGet(key);
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
      assert(globalThis.windowState !== undefined);
      await globalThis.windowState.persistSet(key, schema.parse(value));
    },
    onMutate: async (next) => {
      await queryClient.cancelQueries({ queryKey });
      const previous = queryClient.getQueryData<T>(queryKey);
      queryClient.setQueryData(queryKey, next);
      return { previous };
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
    queryFn: async () => {
      if (globalThis.windowState === undefined) return false;
      return await globalThis.windowState.isFullScreen();
    },
    staleTime: Number.POSITIVE_INFINITY,
    gcTime: Number.POSITIVE_INFINITY,
  });

  useEffect(() => {
    return globalThis.windowState?.onFullScreenChanged((isFullScreen) => {
      queryClient.setQueryData(queryKey, isFullScreen);
    });
  }, [queryClient, queryKey]);

  return isFullScreen ?? false;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
