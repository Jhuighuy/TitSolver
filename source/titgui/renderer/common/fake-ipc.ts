/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import type { z } from "zod";

import { ipcContract, type IpcContract } from "~/shared/ipc/contract";
import {
  type ContractHandlersOf,
  createIpcClient,
  createMethodHandlers,
  eventChannel,
  type EventDef,
  type IpcTransport,
} from "~/shared/ipc/core";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

type FullHandlers = ContractHandlersOf<IpcContract, null>;

/**
 * Test implementations of contract methods. Methods without a fake reject
 * on call, so a test only implements what it exercises.
 */
export type FakeIpcHandlers = {
  [Service in keyof FullHandlers]?: Partial<FullHandlers[Service]>;
};

type ServiceName = keyof IpcContract;

type EventName<Service extends ServiceName> =
  keyof IpcContract[Service]["events"] & string;

type EventPayload<
  Service extends ServiceName,
  Event extends EventName<Service>,
> =
  IpcContract[Service]["events"][Event] extends EventDef<infer Payload>
    ? z.infer<Payload>
    : never;

/**
 * The installed fake bridge: emits contract events into the renderer and
 * uninstalls itself.
 */
export interface FakeIpc {
  /** Emit a contract event, like the main process would. */
  emit: <Service extends ServiceName, Event extends EventName<Service>>(
    serviceName: Service,
    eventName: Event,
    payload: EventPayload<Service, Event>,
  ) => void;

  /** Remove the fake bridge. */
  uninstall: () => void;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
 * Install a fake IPC bridge for the current test: the real contract client
 * (with its argument/result/event validation) over an in-process loopback
 * to the given fake handlers. Call `uninstall` when the test is done.
 */
export function installFakeIpc(handlers: FakeIpcHandlers): FakeIpc {
  // Fill the gaps: every contract method exists, unimplemented ones reject.
  type LooseHandlers = Record<string, Record<string, unknown>>;
  const looseHandlers = handlers as LooseHandlers;
  const fullHandlers = Object.fromEntries(
    Object.entries(ipcContract).map(([serviceName, serviceDef]) => [
      serviceName,
      Object.fromEntries(
        Object.keys(serviceDef.methods).map((methodName) => [
          methodName,
          looseHandlers[serviceName]?.[methodName] ??
            (() => {
              throw new Error(
                `No fake handler for '${serviceName}.${methodName}'.`,
              );
            }),
        ]),
      ),
    ]),
  ) as FullHandlers;

  const methodHandlers = createMethodHandlers(ipcContract, fullHandlers);
  const listeners = new Map<string, Set<(payload: unknown) => void>>();

  const transport: IpcTransport = {
    invoke: async (channel, ...args) => {
      const handle = methodHandlers.get(channel);
      if (handle === undefined) throw new Error(`No channel '${channel}'.`);
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

  globalThis.titgui = createIpcClient(ipcContract, transport);

  return {
    emit: (serviceName, eventName, payload) => {
      const channel = eventChannel(serviceName, eventName);
      for (const listener of listeners.get(channel) ?? []) listener(payload);
    },
    uninstall: () => {
      globalThis.titgui = undefined;
    },
  };
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
