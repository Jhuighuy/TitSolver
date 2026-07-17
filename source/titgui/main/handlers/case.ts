/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { BrowserWindow } from "electron";

import type { CaseManager } from "~/main/case";
import type { CaseFlows } from "~/main/case-flows";
import type { IpcMainHandlers } from "~/main/ipc";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
 * Handlers of the `case` IPC service. Dialog-bearing operations (with the
 * unsaved-changes guard) go through the shared case flows.
 */
export function createCaseHandlers(
  requireCase: () => CaseManager,
  requireFlows: () => CaseFlows,
): IpcMainHandlers["case"] {
  return {
    state: () => requireCase().state(),
    recents: () => requireCase().recents(),
    newCase: async (event) =>
      requireFlows().newCase(BrowserWindow.fromWebContents(event.sender)),
    openCase: async (event) =>
      requireFlows().openCase(BrowserWindow.fromWebContents(event.sender)),
    openRecent: async (event, dir) =>
      requireFlows().openRecent(
        BrowserWindow.fromWebContents(event.sender),
        dir,
      ),
    save: async () => requireCase().save(),
    close: async (event) =>
      requireFlows().close(BrowserWindow.fromWebContents(event.sender)),
    getSpec: async () => requireCase().getSpec(),
    document: async () => requireCase().document(),
    updateTree: async (_event, tree, revision) =>
      requireCase().updateTree(tree, revision),
  };
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
