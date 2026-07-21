/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { BrowserWindow, ipcMain, type IpcMainInvokeEvent } from "electron";
import type { z } from "zod";

import { ipcContract, type IpcContract } from "~/shared/ipc/contract";
import {
  type ContractHandlersOf,
  createMethodHandlers,
  type EventDef,
  eventChannel,
} from "~/shared/ipc/core";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
 * Main-process implementations of the IPC contract methods.
 */
export type IpcMainHandlers = ContractHandlersOf<
  IpcContract,
  IpcMainInvokeEvent
>;

/**
 * Register handlers for all contract methods.
 */
export function exposeIpcHandlers(handlers: IpcMainHandlers) {
  for (const [channel, handle] of createMethodHandlers(ipcContract, handlers)) {
    ipcMain.handle(channel, handle);
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

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
 * Send a contract event to the given window.
 */
export function sendIpcEvent<
  Service extends ServiceName,
  Event extends EventName<Service>,
>(
  window: BrowserWindow,
  serviceName: Service,
  eventName: Event,
  payload: EventPayload<Service, Event>,
) {
  const events = ipcContract[serviceName].events as Readonly<
    Record<string, EventDef>
  >;
  window.webContents.send(
    eventChannel(serviceName, eventName),
    events[eventName].payload.parse(payload),
  );
}

/**
 * Broadcast a contract event to all windows.
 */
export function broadcastIpcEvent<
  Service extends ServiceName,
  Event extends EventName<Service>,
>(
  serviceName: Service,
  eventName: Event,
  payload: EventPayload<Service, Event>,
) {
  for (const window of BrowserWindow.getAllWindows()) {
    sendIpcEvent(window, serviceName, eventName, payload);
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
