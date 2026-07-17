/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import type { HelpService } from "~/main/help";
import type { IpcMainHandlers } from "~/main/ipc";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
 * Handlers of the `help` IPC service.
 */
export function createHelpHandlers(
  requireHelp: () => HelpService,
): IpcMainHandlers["help"] {
  return {
    getSession: () => requireHelp().getSession(),
    addTab: (_event, url) => {
      requireHelp().addTab(url);
    },
    closeTab: (_event, id) => {
      requireHelp().closeTab(id);
    },
    selectTab: (_event, id) => {
      requireHelp().selectTab(id);
    },
    navigateTab: (_event, id, url) => {
      requireHelp().navigateTab(id, url);
    },
  };
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
