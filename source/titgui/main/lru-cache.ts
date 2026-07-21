/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { assert } from "~/shared/utils";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
 * A least-recently-used cache of asynchronously loaded values. Concurrent
 * requests for the same key share one load; failed loads are evicted so they
 * can be retried.
 */
export class AsyncLruCache<Key, Value> {
  private readonly entries = new Map<Key, Promise<Value>>();

  /**
   * Construct a cache with the given capacity.
   */
  public constructor(private readonly capacity: number) {
    assert(capacity > 0);
  }

  /**
   * Check if the value for the given key is cached or being loaded.
   */
  public has(key: Key) {
    return this.entries.has(key);
  }

  /**
   * Get the value for the given key, loading it if necessary.
   */
  public async get(key: Key, load: () => Promise<Value>): Promise<Value> {
    const cached = this.entries.get(key);
    if (cached !== undefined) {
      // Bump the entry to the most-recently-used position.
      this.entries.delete(key);
      this.entries.set(key, cached);
      return cached;
    }

    const entry = load();
    this.entries.set(key, entry);
    entry.catch(() => {
      // Do not cache failures.
      if (this.entries.get(key) === entry) this.entries.delete(key);
    });

    // Evict the least-recently-used entries.
    while (this.entries.size > this.capacity) {
      const oldestKey = this.entries.keys().next().value as Key;
      this.entries.delete(oldestKey);
    }

    return entry;
  }

  /**
   * Drop all entries.
   */
  public clear() {
    this.entries.clear();
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
