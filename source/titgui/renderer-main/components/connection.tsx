/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import {
  createContext,
  type ReactNode,
  useCallback,
  useContext,
  useEffect,
  useMemo,
  useRef,
} from "react";
import useWebSocket from "react-use-websocket";
import { z } from "zod";

import { assert } from "~/shared/utils";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export type Callback = (response: unknown) => void;

export interface Connection {
  sendMessage: (message: unknown, callback?: Callback) => void;
}

const ConnectionContext = createContext<Connection | null>(null);

export function useConnection(): Connection {
  const context = useContext(ConnectionContext);
  assert(context !== null, "Server connection is not available.");
  return context;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

interface ConnectionProviderProps {
  children: ReactNode;
}

export function ConnectionProvider({
  children,
}: Readonly<ConnectionProviderProps>) {
  const awaitingResponseRef = useRef(new Map<string, Callback>());
  const { sendMessage: sendRawMessage, lastMessage } = useWebSocket(
    "ws://localhost:18080/ws",
  );

  const sendMessage = useCallback(
    (message: unknown, callback?: Callback) => {
      const requestID = Math.random().toString(36).slice(8);
      const awaitingResponse = awaitingResponseRef.current;
      awaitingResponse.set(requestID, callback ?? noop);
      sendRawMessage(JSON.stringify({ requestID, message }));
    },
    [sendRawMessage],
  );

  useEffect(() => {
    if (lastMessage === null) return;

    assert(typeof lastMessage.data === "string");
    const { requestID, status, result, repeat } = responseSchema.parse(
      JSON.parse(lastMessage.data),
    );

    const awaitingResponse = awaitingResponseRef.current;
    const callback = awaitingResponse.get(requestID);
    if (callback === undefined) {
      console.warn(`No callback found for request ID: ${requestID}.`);
      return;
    }
    if (!repeat) awaitingResponse.delete(requestID);

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

const responseSchema = z
  .object({
    requestID: z.string(),
    status: z.union([z.literal("success"), z.literal("error")]),
    repeat: z.boolean().optional(),
  })
  .and(
    z.union([
      z.object({ status: z.literal("success"), result: z.unknown() }),
      z.object({ status: z.literal("error"), result: z.string() }),
    ]),
  );

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

function noop() {
  // Do nothing.
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
