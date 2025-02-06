/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import {
  FC,
  ReactNode,
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
 * Python error.
 */
export class PyError extends Error {
  /**
   * Construct a new Python error.
   */
  constructor(
    public readonly type: string,
    public readonly error: string,
    public readonly traceback?: string
  ) {
    super(error);
  }

  /**
   * Get the error message.
   */
  toString() {
    let result = `${this.type}: ${this.error}`;
    if (this.traceback) result += `\n\n${this.traceback}`;
    return result;
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
 * Callback to pass the execution result.
 * @param result The result of the Python code or an error.
 */
export type PyCallback = (result: unknown) => void;

/**
 * Run Python code.
 * @param expression The Python code to run.
 * @param onResponse The callback to run when the code finishes.
 */
export type PyRunCode = (expression: string, onResponse: PyCallback) => void;

/**
 * Access connection to the Python server.
 * @returns A function to run Python code.
 */
export function usePython(): PyRunCode {
  const runCode = useContext(PyConnectionContext);
  assert(runCode !== null, "runCode is null!");
  return runCode;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

const PyConnectionContext = createContext<PyRunCode | null>(null);
const PyResponseSchema = z.union([
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
 * Provide a connection to the Python server to the children.
 */
export const PyConnectionProvider: FC<{ children: ReactNode }> = ({
  children,
}) => {
  const [webSocket, setWebSocket] = useState<WebSocket | null>(null);
  const pendingRequests = useRef(new Map<string, PyCallback>());

  useEffect(() => {
    const ws = new WebSocket(`ws://${window.location.host}/ws`);
    ws.onopen = () => {
      setWebSocket(ws);

      // Ping every 90 seconds to keep the connection alive.
      const pingInterval = 90 * 1000;
      setInterval(() => ws.send(JSON.stringify({ ping: 1 })), pingInterval);
    };
    ws.onclose = () => setWebSocket(null);
    ws.onmessage = (event: MessageEvent) => {
      // Parse and validate the message.
      assert(typeof event.data === "string", "Not a string message.");
      const { requestID, status, result } = PyResponseSchema.parse(
        JSON.parse(event.data)
      );

      // Invoke the callback.
      const callback = pendingRequests.current.get(requestID);
      assert(callback !== undefined, `No callback for request ${requestID}`);
      if (status === "success") {
        callback(result);
      } else {
        assert(status === "error");
        const { type, error, traceback } = result;
        callback(new PyError(type, error, traceback));
      }
      pendingRequests.current.delete(requestID);
    };

    return () => {
      if (ws.readyState === WebSocket.OPEN) ws.close();
    };
  }, []);

  const runCode = useCallback(
    (expression: string, onResponse: PyCallback) => {
      assert(webSocket !== null, "WebSocket is not null!");
      assert(
        webSocket.readyState === WebSocket.OPEN,
        "Socket is not connected!"
      );

      const requestID = Math.random().toString(36).substring(8);
      pendingRequests.current.set(requestID, onResponse);
      webSocket.send(JSON.stringify({ requestID, expression }));
    },
    [webSocket]
  );

  if (!webSocket) return <div>Connecting...</div>;
  return (
    <PyConnectionContext.Provider value={runCode}>
      {children}
    </PyConnectionContext.Provider>
  );
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
