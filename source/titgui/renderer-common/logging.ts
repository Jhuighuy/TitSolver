/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { signal } from "~/renderer-common/signals";

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
  public readonly messages = signal<Message[]>([]);

  public clear() {
    this.messages.set([]);
  }

  public log(...data: unknown[]) {
    this.addMessage("log", ...data);
  }

  public warn(...data: unknown[]) {
    this.addMessage("warning", ...data);
  }

  public err(...data: unknown[]) {
    this.addMessage("error", ...data);
  }

  private addMessage(type: MessageType, ...data: unknown[]) {
    const message = {
      id: this.counter++,
      type,
      text: data.join(" "),
    };
    this.messages.set(
      [...this.messages.get(), message].slice(-NUM_MESSAGES_TO_KEEP),
    );
  }
}

export const logger = new Logger();

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

const NUM_MESSAGES_TO_KEEP = 100;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
