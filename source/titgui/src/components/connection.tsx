/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import {
  createContext,
  type ReactElement,
  useCallback,
  useContext,
  useEffect,
  useMemo,
  useRef,
} from "react";
import useWebSocket from "react-use-websocket";
import { z } from "zod";

import { assert } from "~/utils";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export type Callback = (response: unknown) => void;

export type Connection = {
  sendMessage: (message: unknown, callback?: Callback) => void;
};

const ConnectionContext = createContext<Connection | null>(null);

export function useConnection(): Connection {
  const context = useContext(ConnectionContext);
  assert(context !== null, "Server connection is not available.");
  return context;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

type ConnectionProviderProps = {
  children: ReactElement | ReactElement[];
};

export function ConnectionProvider({
  children,
}: Readonly<ConnectionProviderProps>) {
  const awaitingResponseRef = useRef(new Map<string, Callback>());
  const { sendMessage: sendRawMessage, lastMessage } = useWebSocket(
    `ws://${globalThis.location.host ?? "localhost:8080"}/ws`
  );

  const sendMessage = useCallback(
    (message: unknown, callback?: Callback) => {
      const requestID = Math.random().toString(36).substring(8);
      if (callback) {
        const awaitingResponse = awaitingResponseRef.current;
        awaitingResponse.set(requestID, callback);
      }
      sendRawMessage(JSON.stringify({ requestID, message }));
    },
    [sendRawMessage]
  );

  useEffect(() => {
    if (lastMessage === null) return;

    assert(typeof lastMessage.data === "string");
    const { requestID, status, result } = responseSchema.parse(
      JSON.parse(lastMessage.data)
    );

    const awaitingResponse = awaitingResponseRef.current;
    const callback = awaitingResponse.get(requestID);
    assert(callback !== undefined, "Callback not found.");
    awaitingResponse.delete(requestID);

    switch (status) {
      case "success":
        callback(result);
        break;
      case "error":
        throw new Error(result);
      default:
        assert(false, "Unhandled response status.");
    }
  }, [lastMessage]);

  const connection = useMemo(() => ({ sendMessage }), [sendMessage]);

  return (
    <ConnectionContext.Provider value={connection}>
      {children}
    </ConnectionContext.Provider>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

const responseSchema = z.union([
  z.object({
    requestID: z.string(),
    status: z.literal("error"),
    result: z.string(),
  }),
  z.object({
    requestID: z.string(),
    status: z.literal("success"),
    result: z.unknown(),
  }),
]);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
