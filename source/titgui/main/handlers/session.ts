/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { BrowserWindow, dialog, type OpenDialogOptions } from "electron";

import type { CaseManager } from "~/main/case";
import type { IpcMainHandlers } from "~/main/ipc";
import type { SessionManager } from "~/main/session";
import { ensure } from "~/shared/utils";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
 * Handlers of the `session` IPC service. The session is case-scoped and may
 * be absent — queries report the empty state, actions fail politely.
 */
export function createSessionHandlers(
  getSession: () => SessionManager | undefined,
  requireCase: () => CaseManager,
): IpcMainHandlers["session"] {
  const requireSession = () => {
    const session = getSession();
    ensure(session !== undefined, "No case is open.");
    return session;
  };

  return {
    // No open case means no session — and simply zero frames.
    frameCount: async () => getSession()?.getFrameCount() ?? 0,
    frameTimes: async () => getSession()?.getFrameTimes() ?? [],
    frame: async (_event, index) => requireSession().getFrame(index),
    export: async (event) => {
      const options: OpenDialogOptions = {
        title: "Export Data",
        buttonLabel: "Export",
        properties: ["openDirectory", "createDirectory"],
      };
      const window = BrowserWindow.fromWebContents(event.sender);
      const result =
        window === null
          ? await dialog.showOpenDialog(options)
          : await dialog.showOpenDialog(window, options);
      if (result.canceled) return;
      await requireSession().export(result.filePaths[0]);
    },
    runSolver: async () => {
      // Running requires a clean materialization; the solver would reject
      // an invalid case file anyway, but failing here is clearer.
      const document = await requireCase().document();
      ensure(document !== null, "No case is open.");
      ensure(
        document.materialized.issues.length === 0,
        "The case has validation issues; fix them before running.",
      );
      requireSession().runSolver();
    },
    stopSolver: () => {
      requireSession().stopSolver();
    },
    isSolverRunning: () => getSession()?.isSolverRunning() ?? false,
  };
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
