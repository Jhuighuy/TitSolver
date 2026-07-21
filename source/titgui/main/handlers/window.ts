/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { BrowserWindow, type IpcMainInvokeEvent } from "electron";
import { z } from "zod";

import type { IpcMainHandlers } from "~/main/ipc";
import type { WindowController } from "~/main/window";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
 * Handlers of the `window` IPC service (per-window persisted state and
 * full-screen queries).
 */
export function createWindowHandlers(
  findWindowController: (
    event: IpcMainInvokeEvent,
  ) => WindowController | undefined,
): IpcMainHandlers["window"] {
  return {
    persistGet: (event, key) => {
      return findWindowController(event)?.persist.get(key, z.unknown());
    },
    persistSet: (event, key, value) => {
      findWindowController(event)?.persist.set(key, value);
    },
    isFullScreen: (event) => {
      const window = BrowserWindow.fromWebContents(event.sender);
      return window?.isFullScreen() ?? false;
    },
  };
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
