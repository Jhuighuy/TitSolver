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
