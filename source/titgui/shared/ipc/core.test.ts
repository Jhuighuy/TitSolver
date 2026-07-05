/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { describe, expect, it } from "vitest";
import { z } from "zod";

import {
  type ContractHandlersOf,
  createIpcClient,
  createMethodHandlers,
  event,
  eventChannel,
  IpcError,
  type IpcTransport,
  method,
  methodChannel,
  service,
} from "~/shared/ipc/core";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

const testContract = {
  math: service({
    methods: {
      add: method({ args: [z.number(), z.number()], result: z.number() }),
      greet: method({ args: [z.string().optional()], result: z.string() }),
      fail: method({ result: z.void() }),
      lie: method({ result: z.number() }),
    },
    events: {
      ticked: event(z.number()),
    },
  }),
};

const testHandlers: ContractHandlersOf<typeof testContract, null> = {
  math: {
    add: (_context, a, b) => a + b,
    greet: (_context, name) => `Hello, ${name ?? "world"}!`,
    fail: () => {
      throw new Error("Boom.");
    },
    lie: () => "not a number" as unknown as number,
  },
};

// Wire a client to the handlers through an in-memory transport, emulating the
// `ipcRenderer.invoke` / `webContents.send` round trip.
function connect() {
  const methodHandlers = createMethodHandlers(testContract, testHandlers);
  const listeners = new Map<string, Set<(payload: unknown) => void>>();

  const transport: IpcTransport = {
    invoke: async (channel, ...args) => {
      const handle = methodHandlers.get(channel);
      if (handle === undefined) throw new Error(`No handler for '${channel}'.`);
      return handle(null, ...args);
    },
    on: (channel, listener) => {
      const channelListeners = listeners.get(channel) ?? new Set();
      channelListeners.add(listener);
      listeners.set(channel, channelListeners);
      return () => {
        channelListeners.delete(listener);
      };
    },
  };

  const emit = (channel: string, payload: unknown) => {
    for (const listener of listeners.get(channel) ?? []) listener(payload);
  };

  return { client: createIpcClient(testContract, transport), emit };
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

describe("channel naming", () => {
  it("derives method channels", () => {
    expect(methodChannel("math", "add")).toBe("ipc:math:add");
  });

  it("derives event channels", () => {
    expect(eventChannel("math", "ticked")).toBe("ipc:math:event:ticked");
  });
});

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

describe("method calls", () => {
  it("round-trips arguments and results", async () => {
    const { client } = connect();
    await expect(client.math.add(2, 3)).resolves.toBe(5);
  });

  it("supports optional trailing arguments", async () => {
    const { client } = connect();
    await expect(client.math.greet()).resolves.toBe("Hello, world!");
    await expect(client.math.greet("Tit")).resolves.toBe("Hello, Tit!");
  });

  it("rejects invalid arguments with an IpcError", async () => {
    const { client } = connect();
    const add = client.math.add as unknown as (
      ...args: unknown[]
    ) => Promise<number>;
    await expect(add("2", 3)).rejects.toBeInstanceOf(IpcError);
    await expect(add("2", 3)).rejects.toThrow(/^math\.add: /u);
  });

  it("rejects extra arguments", async () => {
    const { client } = connect();
    const add = client.math.add as unknown as (
      ...args: unknown[]
    ) => Promise<number>;
    await expect(add(1, 2, 3)).rejects.toThrow(
      "math.add: Expected at most 2 argument(s), got 3.",
    );
  });

  it("rethrows handler errors with a clean message", async () => {
    const { client } = connect();
    await expect(client.math.fail()).rejects.toBeInstanceOf(IpcError);
    await expect(client.math.fail()).rejects.toThrow("math.fail: Boom.");
  });

  it("rejects results violating the contract", async () => {
    const { client } = connect();
    await expect(client.math.lie()).rejects.toThrow(
      /^math\.lie: Invalid result:/u,
    );
  });
});

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

describe("events", () => {
  it("delivers validated payloads and honors unsubscription", () => {
    const { client, emit } = connect();
    const channel = eventChannel("math", "ticked");

    const seen: number[] = [];
    const unsubscribe = client.math.onTicked((payload) => {
      seen.push(payload);
    });

    emit(channel, 1);
    expect(seen).toEqual([1]);

    unsubscribe();
    emit(channel, 2);
    expect(seen).toEqual([1]);
  });

  it("rejects payloads violating the contract", () => {
    const { client, emit } = connect();
    const channel = eventChannel("math", "ticked");

    const seen: number[] = [];
    client.math.onTicked((payload) => {
      seen.push(payload);
    });

    expect(() => {
      emit(channel, "not a number");
    }).toThrow();
    expect(seen).toEqual([]);
  });
});

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
