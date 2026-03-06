/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { createContext, useCallback, useMemo, useState } from "react";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

type MessageType = "log" | "warning" | "error";
type Message = {
  type: MessageType;
  time: number;
  text: string;
};

export type Logging = {
  messages: Message[];
  clear: () => void;
  log: (...data: unknown[]) => void;
  warn: (...data: unknown[]) => void;
  err: (...data: unknown[]) => void;
};

export const LoggingContext = createContext<Logging | null>(null);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

type LoggingProviderProps = {
  children: React.ReactNode;
};

export function LoggingProvider({ children }: Readonly<LoggingProviderProps>) {
  const [messages, setMessages] = useState<Message[]>([]);

  const addMessage = useCallback((type: MessageType, ...data: unknown[]) => {
    const message = { type, time: Date.now(), text: data.join(" ") };
    setMessages((prev) => [...prev, message].slice(-NUM_MESSAGES_TO_KEEP));
  }, []);

  const logging = useMemo<Logging>(
    () => ({
      messages,
      clear: () => setMessages([]),
      log: (...data: unknown[]) => addMessage("log", ...data),
      warn: (...data: unknown[]) => addMessage("warning", ...data),
      err: (...data: unknown[]) => addMessage("error", ...data),
    }),
    [messages, addMessage],
  );

  return (
    <LoggingContext.Provider value={logging}>
      {children}
    </LoggingContext.Provider>
  );
}

const NUM_MESSAGES_TO_KEEP = 100;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
