/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { ipcRenderer, type IpcRendererEvent } from "electron";

import { ipcContract, type IpcClient } from "~/shared/ipc/contract";
import { createIpcClient } from "~/shared/ipc/core";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
 * Build the contract client backed by `ipcRenderer`.
 */
export function createPreloadIpcClient(): IpcClient {
  return createIpcClient(ipcContract, {
    invoke: async (channel, ...args) => {
      const result: unknown = await ipcRenderer.invoke(channel, ...args);
      return result;
    },
    on: (channel, listener) => {
      const callback = (_event: IpcRendererEvent, payload: unknown) => {
        listener(payload);
      };
      ipcRenderer.on(channel, callback);
      return () => {
        ipcRenderer.off(channel, callback);
      };
    },
  });
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
