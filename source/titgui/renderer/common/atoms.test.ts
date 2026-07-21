/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { atom, createStore } from "jotai";
import { describe, expect, it } from "vitest";

import { scopedAtom } from "~/renderer/common/atoms";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

describe("scopedAtom", () => {
  it("falls back until written in the current scope", () => {
    const store = createStore();
    const keyAtom = atom("a");
    const scoped = scopedAtom(keyAtom, () => 10);

    expect(store.get(scoped)).toBe(10);
    store.set(scoped, 42);
    expect(store.get(scoped)).toBe(42);
  });

  it("remembers values independently per scope", () => {
    const store = createStore();
    const keyAtom = atom("a");
    const scoped = scopedAtom(keyAtom, () => 0);

    store.set(scoped, 1);
    store.set(keyAtom, "b");
    expect(store.get(scoped)).toBe(0);

    store.set(scoped, 2);
    expect(store.get(scoped)).toBe(2);

    store.set(keyAtom, "a");
    expect(store.get(scoped)).toBe(1);
  });

  it("re-evaluates the fallback while unset", () => {
    const store = createStore();
    const keyAtom = atom("a");
    const sourceAtom = atom(5);
    const scoped = scopedAtom(keyAtom, (get) => get(sourceAtom));

    expect(store.get(scoped)).toBe(5);
    store.set(sourceAtom, 6);
    expect(store.get(scoped)).toBe(6);

    store.set(scoped, 7);
    store.set(sourceAtom, 8);
    expect(store.get(scoped)).toBe(7);
  });

  it("notifies subscribers on scope changes", () => {
    const store = createStore();
    const keyAtom = atom("a");
    const scoped = scopedAtom(keyAtom, () => 0);

    const seen: number[] = [];
    store.sub(scoped, () => {
      seen.push(store.get(scoped));
    });

    store.set(scoped, 1);
    store.set(keyAtom, "b");
    store.set(scoped, 2);
    store.set(keyAtom, "a");
    expect(seen).toEqual([1, 0, 2, 1]);
  });
});

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
