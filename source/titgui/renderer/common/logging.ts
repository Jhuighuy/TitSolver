/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { atom, getDefaultStore } from "jotai";

import { toastManager } from "~/renderer/common/components/toast";

// oxlint-disable no-console -- The logger is the single console mirror point.

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export type MessageType = "log" | "warning" | "error";

export interface Message {
  id: number;
  type: MessageType;
  text: string;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class Logger {
  private counter = 0;
  public readonly messagesAtom = atom<Message[]>([]);

  public clear() {
    getDefaultStore().set(this.messagesAtom, []);
  }

  public log(...data: unknown[]) {
    console.log(...data);
    this.addMessage("log", ...data);
  }

  public warn(...data: unknown[]) {
    console.warn(...data);
    this.addMessage("warning", ...data);
  }

  /**
   * Log an error. Errors also surface as toasts: they are recoverable, but
   * the user must see them without digging through the Logs pane.
   */
  public err(...data: unknown[]) {
    console.error(...data);
    const message = this.addMessage("error", ...data);
    const [title = "Error", ...rest] = message.text.split("\n");
    toastManager.add({
      type: "error",
      title: title.trim(),
      description: rest.join("\n").trim() || undefined,
    });
  }

  private addMessage(type: MessageType, ...data: unknown[]): Message {
    const message = {
      id: this.counter++,
      type,
      text: data.join(" "),
    };
    const store = getDefaultStore();
    store.set(
      this.messagesAtom,
      [...store.get(this.messagesAtom), message].slice(-NUM_MESSAGES_TO_KEEP),
    );
    return message;
  }
}

export const logger = new Logger();

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
 * Route uncaught errors and unhandled rejections to the logger. Call once
 * per window.
 */
export function installGlobalErrorLogging() {
  globalThis.addEventListener("error", (event) => {
    logger.err("Uncaught error.\n", event.message);
  });
  globalThis.addEventListener("unhandledrejection", (event) => {
    logger.err("Unhandled rejection.\n", event.reason);
  });
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

const NUM_MESSAGES_TO_KEEP = 100;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
