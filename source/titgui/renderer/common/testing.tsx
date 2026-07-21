/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { QueryClient, QueryClientProvider } from "@tanstack/react-query";
import { render } from "@testing-library/react";
import type { ReactNode } from "react";

import {
  type MenuAction,
  MenuActionsProvider,
} from "~/renderer/common/components/menu";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
 * Render a component under the app-level providers: a fresh React Query
 * client (no retries, so failures surface immediately) and a menu-pane
 * action registry that records registered actions.
 */
export function renderWithProviders(ui: ReactNode) {
  const queryClient = new QueryClient({
    defaultOptions: { queries: { retry: false } },
  });

  const menuActions: MenuAction[] = [];
  const addAction = (action: MenuAction) => {
    menuActions.push(action);
    return () => {
      const index = menuActions.indexOf(action);
      if (index >= 0) menuActions.splice(index, 1);
    };
  };

  const result = render(
    <QueryClientProvider client={queryClient}>
      <MenuActionsProvider value={{ addAction }}>{ui}</MenuActionsProvider>
    </QueryClientProvider>,
  );

  return { ...result, menuActions };
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
