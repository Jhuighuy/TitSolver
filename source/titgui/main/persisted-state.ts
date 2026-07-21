/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import fs from "node:fs";

import Store from "electron-store";
import { z } from "zod";

import { log } from "~/main/log";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
 * Persisted application state shared by windows.
 *
 * Backed by `electron-store`: every `set` is written to disk atomically
 * right away, so a crash never loses state. Values are validated with zod
 * schemas on read; invalid or missing values fall back.
 */
export class PersistedState {
  private constructor(
    private readonly store: Store,
    private readonly prefix?: string,
  ) {}

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /**
   * Open the persisted state store (`persisted-state.json` in the user data
   * directory). When the store is empty and a legacy file is given, its
   * contents are imported once.
   */
  public static open(legacyPath?: string) {
    const store = new Store({
      name: "persisted-state",
      // Keys are flat strings (e.g. "main.window.x"), not object paths.
      accessPropertiesByDotNotation: false,
      clearInvalidConfig: true,
    });

    if (store.size === 0 && legacyPath !== undefined) {
      PersistedState.import(store, legacyPath);
    }

    return new PersistedState(store);
  }

  // Import state from a legacy persisted state file, ignoring bad content.
  private static import(store: Store, path: string) {
    let content: string;
    try {
      content = fs.readFileSync(path, "utf8");
    } catch {
      return; // File does not exist, do not issue warning.
    }

    let json: unknown;
    try {
      json = JSON.parse(content);
    } catch (error) {
      log.warn(`Failed to parse legacy persisted state at '${path}':`, error);
      return;
    }

    const { success, data } = z.record(z.string(), z.unknown()).safeParse(json);
    if (!success) {
      log.warn(`Invalid legacy persisted state format at '${path}'.`);
      return;
    }

    log.info(`Migrating persisted state from '${path}'.`);
    store.set(data);
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /**
   * Get the prefixed persisted state. Prefixed views read and write the
   * same store, so writes are persisted no matter which view makes them.
   */
  public withPrefix(prefix: string) {
    return new PersistedState(
      this.store,
      this.prefix === undefined ? prefix : `${this.prefix}.${prefix}`,
    );
  }

  /**
   * Get a value from persisted state.
   */
  public get<T>(key: string, schema: z.ZodType<T>): T | undefined;
  public get<T>(key: string, schema: z.ZodType<T>, fallbackValue: T): T;
  public get<T>(
    key: string,
    schema: z.ZodType<T>,
    fallbackValue?: T,
  ): T | undefined {
    if (this.prefix !== undefined) key = `${this.prefix}.${key}`;

    // Query the value.
    // If it does not exist, silently return the fallback.
    const persistedValue = this.store.get(key);
    if (persistedValue === undefined) return fallbackValue;

    // Parse the value.
    // If it is invalid, issue a warning and return the fallback.
    const { success, data: value, error } = schema.safeParse(persistedValue);
    if (!success) {
      log.warn(
        `Ignoring invalid persisted value for '${key}', using fallback:`,
        error.message,
      );
      return fallbackValue;
    }

    // Return the value.
    return value;
  }

  /**
   * Set a value in persisted state. The store is written to disk
   * immediately and atomically.
   */
  public set(key: string, value: unknown) {
    if (this.prefix !== undefined) key = `${this.prefix}.${key}`;
    if (value === undefined) {
      this.store.delete(key);
    } else {
      this.store.set(key, value);
    }
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
