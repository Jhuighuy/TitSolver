/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import fs from "node:fs";
import os from "node:os";
import path from "node:path";

import { afterEach, beforeEach, describe, expect, it, vi } from "vitest";
import { z } from "zod";

import { PersistedState } from "~/main/persisted-state";

vi.mock("~/main/log", () => ({
  log: { info: vi.fn(), warn: vi.fn(), error: vi.fn() },
}));

// An in-memory stand-in for `electron-store` (which needs a live Electron
// app for its file location).
vi.mock("electron-store", () => ({
  default: class FakeStore {
    public static data = new Map<string, unknown>();

    public get size() {
      return FakeStore.data.size;
    }

    public get(key: string) {
      return FakeStore.data.get(key);
    }

    public set(keyOrData: string | Record<string, unknown>, value?: unknown) {
      if (typeof keyOrData === "string") {
        FakeStore.data.set(keyOrData, value);
        return;
      }
      for (const [key, entry] of Object.entries(keyOrData)) {
        FakeStore.data.set(key, entry);
      }
    }

    public delete(key: string) {
      FakeStore.data.delete(key);
    }
  },
}));

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

interface FakeStoreModule {
  default: { data: Map<string, unknown> };
}

async function storeData() {
  const module = (await import("electron-store")) as unknown as FakeStoreModule;
  return module.default.data;
}

let tempDir: string;

beforeEach(async () => {
  (await storeData()).clear();
  tempDir = fs.mkdtempSync(path.join(os.tmpdir(), "titgui-persist-"));
});

afterEach(() => {
  fs.rmSync(tempDir, { recursive: true, force: true });
});

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

describe("PersistedState", () => {
  it("round-trips values through the store", () => {
    const persist = PersistedState.open();
    persist.set("answer", 42);
    expect(persist.get("answer", z.number())).toBe(42);
    expect(persist.get("missing", z.number())).toBeUndefined();
    expect(persist.get("missing", z.number(), 7)).toBe(7);
  });

  it("falls back on schema-invalid values", () => {
    const persist = PersistedState.open();
    persist.set("answer", "not a number");
    expect(persist.get("answer", z.number(), 7)).toBe(7);
  });

  it("prefixes keys through saveable views", async () => {
    const persist = PersistedState.open();
    const view = persist.withPrefix("main").withPrefix("window");
    view.set("x", 10);
    expect((await storeData()).get("main.window.x")).toBe(10);
    expect(view.get("x", z.number())).toBe(10);
  });

  it("deletes keys set to undefined", async () => {
    const persist = PersistedState.open();
    persist.set("gone", 1);
    persist.set("gone", undefined);
    expect((await storeData()).has("gone")).toBe(false);
  });

  it("imports a legacy state file into an empty store", async () => {
    const legacyPath = path.join(tempDir, "persisted-state.json");
    fs.writeFileSync(legacyPath, JSON.stringify({ "main.window.x": 5 }));

    const persist = PersistedState.open(legacyPath);
    expect(persist.withPrefix("main.window").get("x", z.number())).toBe(5);
    expect((await storeData()).get("main.window.x")).toBe(5);
  });

  it("skips the legacy import when the store already has data", () => {
    PersistedState.open().set("existing", true);

    const legacyPath = path.join(tempDir, "persisted-state.json");
    fs.writeFileSync(legacyPath, JSON.stringify({ imported: true }));

    const persist = PersistedState.open(legacyPath);
    expect(persist.get("imported", z.boolean())).toBeUndefined();
  });

  it("tolerates a corrupt legacy file", () => {
    const legacyPath = path.join(tempDir, "persisted-state.json");
    fs.writeFileSync(legacyPath, "{ not json");
    expect(() => PersistedState.open(legacyPath)).not.toThrow();
  });
});

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
