/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import type { IpcClient } from "~/shared/ipc/contract";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

function requireBridge(): IpcClient {
  const client = globalThis.titgui;
  if (client === undefined) {
    throw new Error(
      "IPC bridge is unavailable: the preload script did not run.",
    );
  }
  return client;
}

/**
 * The IPC client. The bridge is resolved lazily per access, so importing
 * this module never throws: the app fails fast on the first call if the
 * preload script did not run, and tests install a fake bridge
 * (`installFakeIpc`) without import-order gymnastics.
 */
export const ipc: IpcClient = new Proxy({} as IpcClient, {
  get: (_target, serviceName) =>
    requireBridge()[serviceName as keyof IpcClient],
});

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
