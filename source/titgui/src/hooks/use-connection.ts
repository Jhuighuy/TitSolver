/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { useCallback, useEffect } from "react";
import useWebSocket from "react-use-websocket";
import { create } from "zustand";
import { z } from "zod";

import { assert, copyDel, copySet } from "~/utils";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export type Callback = (response: unknown) => void;

export type Connection = {
  sendMessage: (message: unknown, callback?: Callback) => void;
  sendMessageAsync: (message: unknown) => Promise<unknown>;
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export function useConnection(): Connection {
  const url = `ws://${globalThis.location.host ?? "localhost:8080"}/ws`;
  const { sendMessage: sendRawMessage, lastMessage } = useWebSocket(url, {
    share: true,
    shouldReconnect: () => true,
  });

  const pushRequest = useRequestStore((s) => s.pushRequest);
  const findRequest = useRequestStore((s) => s.findRequest);

  const sendMessage = useCallback(
    (message: unknown, callback?: Callback) => {
      const requestID = Math.random().toString(36).substring(8);
      sendRawMessage(JSON.stringify({ requestID, message }));
      pushRequest(requestID, callback ?? (() => {}));
    },
    [pushRequest, sendRawMessage]
  );

  const sendMessageAsync = useCallback(
    (message: unknown) =>
      new Promise<unknown>((resolve, reject) => {
        sendMessage(message, (response) => {
          if (response instanceof Error) reject(response);
          else resolve(response);
        });
      }),
    [sendMessage]
  );

  useEffect(() => {
    if (lastMessage === null) return;

    assert(typeof lastMessage.data === "string");
    const { requestID, status, result, repeat } = responseSchema.parse(
      JSON.parse(lastMessage.data)
    );

    const remove = !repeat;
    const callback = findRequest(requestID, remove);
    if (!callback) return;

    switch (status) {
      case "success":
        callback(result);
        break;
      case "error":
        callback(new Error(String(result)));
        break;
      default:
        assert(false, "Unhandled response status.");
    }
  }, [lastMessage, findRequest]);

  return {
    sendMessage,
    sendMessageAsync,
  };
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

type RequestStore = {
  requests: Map<string, Callback>;
  pushRequest: (id: string, callback: Callback) => void;
  findRequest: (id: string, remove: boolean) => Callback | undefined;
};

const useRequestStore = create<RequestStore>((set, get) => ({
  requests: new Map(),
  pushRequest: (id, callback) => {
    set((state) => ({ requests: copySet(state.requests, id, callback) }));
  },
  findRequest: (id, remove) => {
    const callback = get().requests.get(id);
    if (!callback) return;
    if (remove) set((state) => ({ requests: copyDel(state.requests, id) }));
    return callback;
  },
}));

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

const responseSchema = z
  .object({
    requestID: z.string(),
    repeat: z.boolean().optional(),
  })
  .and(
    z.union([
      z.object({ status: z.literal("success"), result: z.unknown() }),
      z.object({ status: z.literal("error"), result: z.string() }),
    ])
  );

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
