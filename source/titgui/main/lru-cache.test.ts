/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { describe, expect, it } from "vitest";

import { AsyncLruCache } from "~/main/lru-cache";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Loader that resolves after a microtask, like a real asynchronous load.
function loadValue<Value>(value: Value) {
  return async () => {
    await Promise.resolve();
    return value;
  };
}

// Loader that fails after a microtask.
const loadFailure = async (): Promise<number> => {
  await Promise.resolve();
  throw new Error("Boom.");
};

describe("AsyncLruCache", () => {
  it("loads each key once and shares concurrent loads", async () => {
    const cache = new AsyncLruCache<number, number>(4);
    let loads = 0;
    const load = async (key: number) => {
      loads += 1;
      await Promise.resolve();
      return key * 10;
    };

    const [a, b] = await Promise.all([
      cache.get(1, async () => load(1)),
      cache.get(1, async () => load(1)),
    ]);
    expect(a).toBe(10);
    expect(b).toBe(10);
    expect(loads).toBe(1);

    await expect(cache.get(1, async () => load(1))).resolves.toBe(10);
    expect(loads).toBe(1);
  });

  it("evicts the least-recently-used entry", async () => {
    const cache = new AsyncLruCache<number, number>(2);

    await cache.get(1, loadValue(1));
    await cache.get(2, loadValue(2));
    await cache.get(1, loadValue(1)); // Bump key 1.
    await cache.get(3, loadValue(3)); // Evicts key 2.

    expect(cache.has(1)).toBe(true);
    expect(cache.has(2)).toBe(false);
    expect(cache.has(3)).toBe(true);
  });

  it("does not cache failed loads", async () => {
    const cache = new AsyncLruCache<number, number>(2);

    await expect(cache.get(1, loadFailure)).rejects.toThrow("Boom.");
    // The rejection is evicted asynchronously.
    await Promise.resolve();

    expect(cache.has(1)).toBe(false);
    await expect(cache.get(1, loadValue(42))).resolves.toBe(42);
  });

  it("clears all entries", async () => {
    const cache = new AsyncLruCache<number, number>(2);
    await cache.get(1, loadValue(1));
    cache.clear();
    expect(cache.has(1)).toBe(false);
  });
});

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
