/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import {
  type FC,
  type ReactNode,
  createContext,
  useCallback,
  useContext,
  useEffect,
  useRef,
  useState,
} from "react";
import { z } from "zod";

import { assert } from "~/utils";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
 * Callback to pass the response.
 * @param response Response from the server.
 */
export type Callback = (response: unknown) => void;

/**
 * Send the message to the server.
 * @param message The message to send.
 * @param callback The callback to pass the response.
 */
export type SendMessage = (expression: string, callback: Callback) => void;

/**
 * Access connection to the server.
 * @returns A function to send messages.
 */
export function useServer(): SendMessage {
  const result = useContext(ConnectionContext);
  assert(result !== null, "Server connection is not available.");
  return result;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

const ConnectionContext = createContext<SendMessage | null>(null);
const ResponseSchema = z.union([
  z.object({
    requestID: z.string(),
    status: z.literal("error"),
    result: z.object({
      type: z.string(),
      error: z.string(),
      traceback: z.string().optional(),
    }),
  }),
  z.object({
    requestID: z.string(),
    status: z.literal("success"),
    result: z.unknown(),
  }),
]);

/**
 * Provide a connection to the server to the child components.
 */
export const ConnectionProvider: FC<{ children: ReactNode }> = ({
  children,
}) => {
  const [webSocket, setWebSocket] = useState<WebSocket | null>(null);
  const pendingRequests = useRef(new Map<string, Callback>());

  useEffect(() => {
    const ws = new WebSocket(`ws://${window.location.host}/ws`);
    ws.onopen = () => setWebSocket(ws);
    ws.onclose = () => setWebSocket(null);
    ws.onmessage = (event: MessageEvent) => {
      // Parse and validate the message.
      assert(typeof event.data === "string", "Not a string message.");
      const { requestID, status, result } = ResponseSchema.parse(
        JSON.parse(event.data)
      );

      // Invoke the callback.
      const callback = pendingRequests.current.get(requestID);
      assert(callback !== undefined, "Callback not found.");
      if (status === "success") {
        callback(result);
      } else {
        assert(status === "error");
        const { type, error, traceback } = result;
        callback(new Error(`${type}: ${error}\n${traceback}`));
      }
      pendingRequests.current.delete(requestID);
    };

    return () => {
      if (ws.readyState === WebSocket.OPEN) ws.close();
    };
  }, []);

  const sendMessage = useCallback(
    (message: string, callback: Callback) => {
      assert(webSocket !== null, "WebSocket is not null!");
      assert(
        webSocket.readyState === WebSocket.OPEN,
        "Socket is not connected!"
      );

      const requestID = Math.random().toString(36).substring(8);
      pendingRequests.current.set(requestID, callback);
      webSocket.send(JSON.stringify({ requestID, message }));
    },
    [webSocket]
  );

  if (!webSocket) return <div>Connecting...</div>;
  return (
    <ConnectionContext.Provider value={sendMessage}>
      {children}
    </ConnectionContext.Provider>
  );
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
