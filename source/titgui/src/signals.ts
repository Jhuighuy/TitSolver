/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

type Slot = () => void;

export type SignalSource<T> = {
  get(): T;
  subscribe(slot: Slot): () => void;
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class Signal<T> implements SignalSource<T> {
  private value: T;
  private readonly slots = new Set<Slot>();

  public constructor(initialValue: T) {
    this.value = initialValue;
  }

  public get() {
    return this.value;
  }

  public set(value: T) {
    if (Object.is(this.value, value)) return;
    this.value = value;
    for (const slot of this.slots) slot();
  }

  public subscribe(slot: Slot) {
    this.slots.add(slot);
    return () => this.slots.delete(slot);
  }
}

export function signal<T>(value: T) {
  return new Signal(value);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class DerivedSignal<T> implements SignalSource<T> {
  private value: T;
  private readonly slots = new Set<Slot>();
  private readonly unsubscribe: Array<() => void>;

  public constructor(compute: () => T, sources: Array<SignalSource<unknown>>) {
    this.value = compute();
    this.unsubscribe = sources.map((source) =>
      source.subscribe(() => {
        const next = compute();
        if (Object.is(this.value, next)) return;
        this.value = next;
        for (const slot of this.slots) slot();
      }),
    );
  }

  public get() {
    return this.value;
  }

  public subscribe(slot: Slot) {
    this.slots.add(slot);
    return () => this.slots.delete(slot);
  }

  public dispose() {
    for (const unsubscribe of this.unsubscribe) unsubscribe();
    this.unsubscribe.length = 0;
    this.slots.clear();
  }
}

export function derived<T>(
  compute: () => T,
  sources: Array<SignalSource<unknown>>,
) {
  return new DerivedSignal(compute, sources);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class ScopedSignal<T> implements SignalSource<T> {
  private readonly key: () => string;
  private readonly fallback: () => T;
  private readonly signal: Signal<T>;
  private readonly byScope = new Map<string, T>();

  public constructor(
    fallback: () => T,
    keySources: Array<SignalSource<unknown>>,
  ) {
    this.fallback = fallback;
    this.key = () => keySources.map((source) => source.get()).join(":");
    this.signal = new Signal(this.get());
    for (const keySource of keySources) {
      keySource.subscribe(() => this.signal.set(this.get()));
    }
  }

  public get() {
    return this.byScope.get(this.key()) ?? this.fallback();
  }

  public set(value: T) {
    this.byScope.set(this.key(), value);
    this.signal.set(value);
  }

  public subscribe(slot: Slot) {
    return this.signal.subscribe(slot);
  }
}

export function scoped<T>(
  fallback: () => T,
  keySources: Array<SignalSource<unknown>>,
) {
  return new ScopedSignal(fallback, keySources);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
