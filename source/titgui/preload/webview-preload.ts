/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { ipcRenderer } from "electron";

import { WEBVIEW_OPEN_IN_TAB_CHANNEL } from "~/shared/channels";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

globalThis.addEventListener(
  "auxclick",
  (event) => {
    if (event.button !== 1) return;

    const target = event.target;
    if (!(target instanceof Element)) return;

    const anchor = target.closest("a[href]");
    if (anchor === null) return;

    let url: URL;
    try {
      url = new URL(
        anchor.getAttribute("href") ?? "",
        globalThis.location.href,
      );
    } catch {
      return;
    }

    event.preventDefault();
    event.stopPropagation();
    ipcRenderer.sendToHost(WEBVIEW_OPEN_IN_TAB_CHANNEL, url.toString());
  },
  true,
);

ipcRenderer.on(WEBVIEW_OPEN_IN_TAB_CHANNEL, (_event, url: unknown) => {
  if (typeof url !== "string" || url.trim() === "") return;
  ipcRenderer.sendToHost(WEBVIEW_OPEN_IN_TAB_CHANNEL, url);
});

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
