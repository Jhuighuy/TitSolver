/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { useQuery, useQueryClient } from "@tanstack/react-query";
import { useEffect, useMemo } from "react";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export function useTabs() {
  const queryClient = useQueryClient();
  const queryKey = useMemo(() => ["session"], []);

  const { data: session } = useQuery({
    queryKey,
    queryFn: async () => await globalThis.help?.getSession(),
    staleTime: Number.POSITIVE_INFINITY,
    gcTime: Number.POSITIVE_INFINITY,
  });

  useEffect(() => {
    return globalThis.help?.onSessionChanged((session) => {
      queryClient.setQueryData(queryKey, session);
    });
  }, [queryClient, queryKey]);

  return {
    ...(session ?? {
      activeTabID: undefined,
      tabs: [],
    }),
    addTab: (url?: string) => {
      void globalThis.help?.addTab(url);
    },
    closeTab: (id: number) => {
      void globalThis.help?.closeTab(id);
    },
    selectTab: (id: number) => {
      void globalThis.help?.selectTab(id);
    },
    navigateTab: (id: number, url?: string) => {
      void globalThis.help?.navigateTab(id, url);
    },
  };
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
