/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import type { z } from "zod";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
 * Definition of a request/response IPC method.
 */
export interface MethodDef<
  Args extends readonly z.ZodType[] = readonly z.ZodType[],
  Result extends z.ZodType = z.ZodType,
> {
  /** Argument schemas, one per positional argument. */
  readonly args: Args;

  /** Result schema. */
  readonly result: Result;
}

/**
 * Define a request/response IPC method.
 */
export function method<Result extends z.ZodType>(def: {
  result: Result;
}): MethodDef<[], Result>;
export function method<
  const Args extends readonly z.ZodType[],
  Result extends z.ZodType,
>(def: { args: Args; result: Result }): MethodDef<Args, Result>;
export function method(def: {
  args?: readonly z.ZodType[];
  result: z.ZodType;
}): MethodDef {
  return { args: def.args ?? [], result: def.result };
}

/**
 * Definition of a push (main to renderer) IPC event.
 */
export interface EventDef<Payload extends z.ZodType = z.ZodType> {
  /** Event payload schema. */
  readonly payload: Payload;
}

/**
 * Define a push (main to renderer) IPC event.
 */
export function event<Payload extends z.ZodType>(
  payload: Payload,
): EventDef<Payload> {
  return { payload };
}

/**
 * Definition of an IPC service: a group of related methods and events.
 */
export interface ServiceDef {
  /** Request/response methods. */
  readonly methods: Readonly<Record<string, MethodDef>>;

  /** Push events. */
  readonly events: Readonly<Record<string, EventDef>>;
}

/**
 * Define an IPC service.
 */
export function service<const Service extends ServiceDef>(
  def: Service,
): Service {
  return def;
}

/**
 * Definition of an IPC contract: a group of services.
 */
export type ContractDef = Readonly<Record<string, ServiceDef>>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
 * Channel name for a contract method.
 */
export function methodChannel(serviceName: string, methodName: string) {
  return `ipc:${serviceName}:${methodName}`;
}

/**
 * Channel name for a contract event.
 */
export function eventChannel(serviceName: string, eventName: string) {
  return `ipc:${serviceName}:event:${eventName}`;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
 * Positional argument types for the given argument schemas. Trailing
 * arguments whose schemas accept `undefined` become optional parameters.
 */
export type ArgsOf<Args extends readonly z.ZodType[]> = Args extends readonly []
  ? []
  : Args extends readonly [
        ...infer Init extends readonly z.ZodType[],
        infer Last extends z.ZodType,
      ]
    ? undefined extends z.infer<Last>
      ? [...ArgsOf<Init>, z.infer<Last>?]
      : { -readonly [Index in keyof Args]: z.infer<Args[Index]> }
    : never;

/**
 * Client-side function type of a contract method.
 */
export type MethodClientOf<Method extends MethodDef> = (
  ...args: ArgsOf<Method["args"]>
) => Promise<z.infer<Method["result"]>>;

/**
 * Client-side listener type of a contract event.
 */
export type EventListenerOf<Event extends EventDef> = (
  payload: z.infer<Event["payload"]>,
) => void;

/**
 * Client-side API of a contract service: one async function per method, and
 * one `on<Event>` subscription function (returning an unsubscribe function)
 * per event.
 */
export type ServiceClientOf<Service extends ServiceDef> = {
  readonly [Method in keyof Service["methods"]]: MethodClientOf<
    Service["methods"][Method]
  >;
} & {
  readonly [Event in keyof Service["events"] &
    string as `on${Capitalize<Event>}`]: (
    listener: EventListenerOf<Service["events"][Event]>,
  ) => () => void;
};

/**
 * Client-side API of a contract.
 */
export type IpcClientOf<Contract extends ContractDef> = {
  readonly [Service in keyof Contract]: ServiceClientOf<Contract[Service]>;
};

/**
 * Handler type of a contract method.
 */
export type MethodHandlerOf<Method extends MethodDef, Context> = (
  context: Context,
  ...args: ArgsOf<Method["args"]>
) => z.infer<Method["result"]> | Promise<z.infer<Method["result"]>>;

/**
 * Handler implementations for all methods of a contract.
 */
export type ContractHandlersOf<Contract extends ContractDef, Context> = {
  readonly [Service in keyof Contract]: {
    readonly [Method in keyof Contract[Service]["methods"]]: MethodHandlerOf<
      Contract[Service]["methods"][Method],
      Context
    >;
  };
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
 * Wire envelope of a method response. Handlers never throw across the IPC
 * boundary, since Electron mangles thrown errors; failures are encoded
 * explicitly instead.
 */
export type Envelope<Value = unknown> =
  | { readonly ok: true; readonly value: Value }
  | { readonly ok: false; readonly message: string };

// Check if the value is a well-formed envelope.
function isEnvelope(value: unknown): value is Envelope {
  if (typeof value !== "object" || value === null) return false;
  const record = value as Record<string, unknown>;
  if (record.ok === true) return true;
  return record.ok === false && typeof record.message === "string";
}

/**
 * Error reported by an IPC method call.
 */
export class IpcError extends Error {
  /**
   * Construct an IPC error.
   */
  public constructor(
    /** Name of the service the failed method belongs to. */
    public readonly serviceName: string,
    /** Name of the failed method. */
    public readonly methodName: string,
    /** Failure description. */
    message: string,
  ) {
    super(`${serviceName}.${methodName}: ${message}`);
    this.name = "IpcError";
  }
}

// Extract a human-readable message from a thrown value.
function errorMessage(error: unknown) {
  return error instanceof Error ? error.message : String(error);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
 * Channel-level method handler: validates arguments, invokes the
 * implementation, and encodes the outcome into an envelope.
 */
export type ChannelHandler<Context> = (
  context: Context,
  ...args: unknown[]
) => Promise<Envelope>;

/**
 * Build the channel-to-handler map for the given contract implementation.
 */
export function createMethodHandlers<Contract extends ContractDef, Context>(
  contract: Contract,
  handlers: ContractHandlersOf<Contract, Context>,
): Map<string, ChannelHandler<Context>> {
  type LooseHandlers = Readonly<
    Record<
      string,
      Readonly<
        Record<string, (context: Context, ...args: unknown[]) => unknown>
      >
    >
  >;
  const looseHandlers = handlers as unknown as LooseHandlers;

  const channelHandlers = new Map<string, ChannelHandler<Context>>();
  for (const [serviceName, serviceDef] of Object.entries(contract)) {
    for (const [methodName, methodDef] of Object.entries(serviceDef.methods)) {
      const handler = looseHandlers[serviceName][methodName];
      channelHandlers.set(
        methodChannel(serviceName, methodName),
        async (context, ...rawArgs) => {
          try {
            if (rawArgs.length > methodDef.args.length) {
              throw new Error(
                `Expected at most ${methodDef.args.length} argument(s), got ${rawArgs.length}.`,
              );
            }
            const args = methodDef.args.map((schema, index) =>
              schema.parse(rawArgs[index]),
            );
            const value = await handler(context, ...args);
            return { ok: true, value };
          } catch (error) {
            return { ok: false, message: errorMessage(error) };
          }
        },
      );
    }
  }
  return channelHandlers;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
 * Transport used by the client to reach the main process.
 */
export interface IpcTransport {
  /** Invoke a method channel. */
  invoke: (channel: string, ...args: unknown[]) => Promise<unknown>;

  /** Subscribe to an event channel. Returns an unsubscribe function. */
  on: (channel: string, listener: (payload: unknown) => void) => () => void;
}

/**
 * Build the client-side API for the given contract. Method results and event
 * payloads are validated against the contract schemas; envelope failures are
 * rethrown as `IpcError`s.
 */
export function createIpcClient<Contract extends ContractDef>(
  contract: Contract,
  transport: IpcTransport,
): IpcClientOf<Contract> {
  const client: Record<string, Record<string, unknown>> = {};
  for (const [serviceName, serviceDef] of Object.entries(contract)) {
    const serviceApi: Record<string, unknown> = {};

    for (const [methodName, methodDef] of Object.entries(serviceDef.methods)) {
      const channel = methodChannel(serviceName, methodName);
      serviceApi[methodName] = async (...args: unknown[]) => {
        const response = await transport.invoke(channel, ...args);
        if (!isEnvelope(response)) {
          throw new IpcError(
            serviceName,
            methodName,
            "Malformed response envelope.",
          );
        }
        if (!response.ok) {
          throw new IpcError(serviceName, methodName, response.message);
        }

        const { success, data, error } = methodDef.result.safeParse(
          response.value,
        );
        if (!success) {
          throw new IpcError(
            serviceName,
            methodName,
            `Invalid result: ${error.message}`,
          );
        }
        return data;
      };
    }

    for (const [eventName, eventDef] of Object.entries(serviceDef.events)) {
      const channel = eventChannel(serviceName, eventName);
      serviceApi[`on${capitalize(eventName)}`] = (
        listener: (payload: unknown) => void,
      ) =>
        transport.on(channel, (rawPayload) => {
          listener(eventDef.payload.parse(rawPayload));
        });
    }

    client[serviceName] = serviceApi;
  }
  return client as unknown as IpcClientOf<Contract>;
}

function capitalize(name: string) {
  return name.charAt(0).toUpperCase() + name.slice(1);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
