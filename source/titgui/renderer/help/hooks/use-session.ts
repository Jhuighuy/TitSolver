/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { useQuery, useQueryClient } from "@tanstack/react-query";
import { useMemo, useEffect } from "react";

import { ipc } from "~/renderer/common/ipc";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export function useSession() {
  const queryClient = useQueryClient();
  const queryKey = useMemo(() => ["session"], []);

  const { data: session } = useQuery({
    queryKey,
    queryFn: async () => ipc.help.getSession(),
    staleTime: Number.POSITIVE_INFINITY,
    gcTime: Number.POSITIVE_INFINITY,
  });

  useEffect(() => {
    return ipc.help.onSessionChanged((session) => {
      queryClient.setQueryData(queryKey, session);
    });
  }, [queryClient, queryKey]);

  return {
    tabs: session?.tabs ?? [],
    activeTabID: session?.activeTabID,
    addTab: (url?: string) => {
      void ipc.help.addTab(url);
    },
    closeTab: (id: number) => {
      void ipc.help.closeTab(id);
    },
    selectTab: (id: number) => {
      void ipc.help.selectTab(id);
    },
    navigateTab: (id: number, url?: string) => {
      void ipc.help.navigateTab(id, url);
    },
  };
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
