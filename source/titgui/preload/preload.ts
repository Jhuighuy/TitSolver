/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { contextBridge, ipcRenderer, type IpcRendererEvent } from "electron";

import {
  HELP_OPEN_CHANNEL,
  THEME_GET_CHANNEL,
  THEME_SET_CHANNEL,
  WINDOW_FULL_SCREEN_CHANGED_CHANNEL,
  WINDOW_IS_FULL_SCREEN_CHANNEL,
  WINDOW_PERSIST_GET_CHANNEL,
  WINDOW_PERSIST_SET_CHANNEL,
} from "~/shared/channels";
import type { Theme } from "~/shared/theme";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

contextBridge.exposeInMainWorld("theme", {
  get() {
    return ipcRenderer.invoke(THEME_GET_CHANNEL);
  },
  set(theme: Theme) {
    return ipcRenderer.invoke(THEME_SET_CHANNEL, theme);
  },
});

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

type FullScreenListener = (isFullScreen: boolean) => void;

contextBridge.exposeInMainWorld("windowState", {
  persistGet(key: string) {
    return ipcRenderer.invoke(WINDOW_PERSIST_GET_CHANNEL, key);
  },
  persistSet(key: string, value: unknown) {
    return ipcRenderer.invoke(WINDOW_PERSIST_SET_CHANNEL, key, value);
  },
  isFullScreen() {
    return ipcRenderer.invoke(WINDOW_IS_FULL_SCREEN_CHANNEL);
  },
  onFullScreenChanged(listener: FullScreenListener) {
    const callback = (_event: IpcRendererEvent, isFullScreen: boolean) => {
      listener(isFullScreen);
    };
    ipcRenderer.on(WINDOW_FULL_SCREEN_CHANGED_CHANNEL, callback);
    return () => ipcRenderer.off(WINDOW_FULL_SCREEN_CHANGED_CHANNEL, callback);
  },
});

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

contextBridge.exposeInMainWorld("help", {
  open(path?: string) {
    return ipcRenderer.invoke(HELP_OPEN_CHANNEL, path);
  },
});

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
