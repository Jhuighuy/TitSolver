/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { contextBridge, type IpcRendererEvent, ipcRenderer } from "electron";

import {
  IPC_FULL_SCREEN_CHANGED,
  IPC_HELP_OPEN,
  IPC_IS_FULL_SCREEN,
  IPC_PERSIST_GET,
  IPC_PERSIST_SET,
} from "~/constants";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

type FullScreenListener = (isFullScreen: boolean) => void;

contextBridge.exposeInMainWorld("windowState", {
  isFullScreen() {
    return ipcRenderer.invoke(IPC_IS_FULL_SCREEN) as Promise<boolean>;
  },
  onFullScreenChanged(listener: FullScreenListener) {
    const callback = (_event: IpcRendererEvent, isFullScreen: boolean) =>
      listener(isFullScreen);

    ipcRenderer.on(IPC_FULL_SCREEN_CHANGED, callback);
    return () => ipcRenderer.off(IPC_FULL_SCREEN_CHANGED, callback);
  },
});

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

contextBridge.exposeInMainWorld("persistedState", {
  get(key: string) {
    return ipcRenderer.invoke(IPC_PERSIST_GET, key) as Promise<unknown>;
  },
  set(key: string, value: unknown) {
    return ipcRenderer.invoke(IPC_PERSIST_SET, key, value) as Promise<void>;
  },
});

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

contextBridge.exposeInMainWorld("help", {
  open() {
    return ipcRenderer.invoke(IPC_HELP_OPEN) as Promise<void>;
  },
});

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
