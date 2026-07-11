/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import fs from "node:fs";

import { z } from "zod";

import { log } from "~/main/log";
import { assert } from "~/shared/utils";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
 * Storage for persisted state shared by windows.
 */
export class PersistedState {
  private constructor(
    private readonly path: string | undefined,
    private readonly prefix: string | undefined,
    private readonly data: z.infer<typeof persistedStateSchema>,
  ) {}

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /**
   * Load persisted state from the given path. When the file does not exist
   * and a legacy path is given, the legacy file is read instead — state is
   * migrated on the next save, which always writes to `path`.
   */
  public static load(path: string, legacyPath?: string) {
    let data = PersistedState.read(path);
    if (data === undefined && legacyPath !== undefined) {
      data = PersistedState.read(legacyPath);
      if (data !== undefined) {
        log.info(`Migrating persisted state from '${legacyPath}'.`);
      }
    }
    return new PersistedState(path, undefined, data ?? {});
  }

  // Read and parse a persisted state file; `undefined` if it does not exist.
  private static read(path: string) {
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
      log.warn(
        `Failed to parse persisted state from '${path}', using empty state:`,
        error,
      );
      return {};
    }

    const {
      success,
      data: state,
      error,
    } = persistedStateSchema.safeParse(json);
    if (!success) {
      log.warn(
        `Invalid persisted state format in '${path}', using empty state:`,
        error.message,
      );
      return {};
    }

    return state;
  }

  /**
   * Save the persisted state to disk.
   */
  public save() {
    assert(this.path !== undefined);
    try {
      fs.writeFileSync(
        this.path,
        JSON.stringify(this.data, undefined, 2),
        "utf8",
      );
    } catch (error) {
      log.error(`Failed to save persisted state to '${this.path}':`, error);
    }
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /**
   * Get the prefixed persisted state.
   */
  public withPrefix(prefix: string) {
    return new PersistedState(undefined, prefix, this.data);
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
    const persistedValue = this.data[key];
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
   * Set a value in persisted state.
   */
  public set(key: string, value: unknown) {
    if (this.prefix !== undefined) key = `${this.prefix}.${key}`;
    this.data[key] = value;
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

const persistedStateSchema = z.record(z.string(), z.unknown());

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
