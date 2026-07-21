/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { EventEmitter } from "node:events";

import { afterEach, beforeEach, describe, expect, it, vi } from "vitest";

import type { Installation } from "~/main/installation";
import { broadcastIpcEvent } from "~/main/ipc";
import { SessionManager } from "~/main/session";

vi.mock("~/main/ipc", () => ({ broadcastIpcEvent: vi.fn() }));
vi.mock("~/main/log", () => ({
  log: { info: vi.fn(), warn: vi.fn(), error: vi.fn() },
}));
vi.mock("pidusage", () => ({
  default: Object.assign(
    vi.fn(async () => ({ cpu: 12, memory: 2048 })),
    {
      clear: vi.fn(),
    },
  ),
}));

// ---- Fake native storage. ----------------------------------------------------

let storageState: {
  failOpen: boolean;
  openedPaths: string[];
  dataVersion: number;
  frameCount: number;
  frameReads: number[];
} = {
  failOpen: false,
  openedPaths: [],
  dataVersion: 1,
  frameCount: 0,
  frameReads: [],
};

vi.mock("~/bindings", () => ({
  openStorage: vi.fn(async (path: string) => {
    if (storageState.failOpen) throw new Error("No storage yet.");
    storageState.openedPaths.push(path);
    const series = {
      frameCount: async () => storageState.frameCount,
      frameTimes: async () =>
        new Float64Array(storageState.frameCount).map((_value, index) => index),
      readFrame: async (index: number) => {
        storageState.frameReads.push(index);
        return {
          rho: {
            type: { kind: "float32_t", rank: 0, dim: 2 },
            data: new Float32Array([index]),
          },
        };
      },
      export: async () => {},
    };
    return {
      dataVersion: async () => storageState.dataVersion,
      seriesCount: async () => (storageState.frameCount > 0 ? 1 : 0),
      lastSeries: async () => series,
    };
  }),
}));

// ---- Fake solver process. ----------------------------------------------------

class FakeChild extends EventEmitter {
  public pid = 4242;
  public stdout = Object.assign(new EventEmitter(), {
    setEncoding: () => {},
  });
  public stderr = Object.assign(new EventEmitter(), {
    setEncoding: () => {},
  });
  public kill = vi.fn(() => {
    this.emit("exit", 0, null);
    return true;
  });
}

let spawnedChildren: FakeChild[] = [];
let spawnCalls: { command: string; args: string[]; cwd?: string }[] = [];

vi.mock("node:child_process", () => ({
  spawn: vi.fn((command: string, args: string[], options: { cwd?: string }) => {
    spawnCalls.push({ command, args, cwd: options.cwd });
    const child = new FakeChild();
    spawnedChildren.push(child);
    return child;
  }),
}));

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

const install = { solverPath: "/fake/bin/titwcsph" } as Installation;
const broadcast = vi.mocked(broadcastIpcEvent);

function broadcasts(eventName: string) {
  return broadcast.mock.calls.filter(([, event]) => event === eventName);
}

let session: SessionManager;

beforeEach(() => {
  vi.clearAllMocks();
  storageState = {
    failOpen: false,
    openedPaths: [],
    dataVersion: 1,
    frameCount: 0,
    frameReads: [],
  };
  spawnedChildren = [];
  spawnCalls = [];
  session = new SessionManager(install, "/tmp/case");
});

afterEach(() => {
  session.stop();
});

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

describe("SessionManager storage", () => {
  it("opens the storage inside the case directory", async () => {
    await session.start();
    expect(storageState.openedPaths).toEqual(["/tmp/case/particles.ttdb"]);
    expect(await session.getFrameCount()).toBe(0);
    expect(await session.getFrameTimes()).toEqual([]);
  });

  it("serves and caches frames with neighbor prefetch", async () => {
    storageState.frameCount = 3;
    await session.start();
    expect(await session.getFrameCount()).toBe(3);

    await session.getFrame(1);
    // The requested frame is read, then its neighbors.
    await vi.waitFor(() => {
      expect(storageState.frameReads.toSorted((a, b) => a - b)).toEqual([
        0, 1, 2,
      ]);
    });

    // A repeated request is a cache hit.
    await session.getFrame(1);
    expect(storageState.frameReads.filter((index) => index === 1)).toHaveLength(
      1,
    );
  });

  it("survives a missing storage and self-heals on the next poll", async () => {
    vi.useFakeTimers();
    try {
      storageState.failOpen = true;
      await session.start();
      expect(await session.getFrameCount()).toBe(0);

      // The storage appears (e.g. the first solver run created it).
      storageState.failOpen = false;
      storageState.frameCount = 2;
      await vi.advanceTimersByTimeAsync(1100);

      expect(broadcasts("storageChanged")).toHaveLength(1);
      expect(await session.getFrameCount()).toBe(2);
    } finally {
      vi.useRealTimers();
    }
  });

  it("broadcasts a change when the data version moves", async () => {
    vi.useFakeTimers();
    try {
      await session.start();
      // First poll snapshots the version, the second sees no change.
      await vi.advanceTimersByTimeAsync(2100);
      expect(broadcasts("storageChanged")).toHaveLength(0);

      storageState.dataVersion = 2;
      await vi.advanceTimersByTimeAsync(1100);
      expect(broadcasts("storageChanged")).toHaveLength(1);
    } finally {
      vi.useRealTimers();
    }
  });
});

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

describe("SessionManager solver", () => {
  it("spawns the solver on the case file, once", () => {
    session.runSolver();
    session.runSolver();

    expect(spawnCalls).toEqual([
      {
        command: "/fake/bin/titwcsph",
        args: ["/tmp/case/case.yaml"],
        cwd: "/tmp/case",
      },
    ]);
    expect(session.isSolverRunning()).toBe(true);
  });

  it("forwards output and exit as solver events", () => {
    session.runSolver();
    const child = spawnedChildren[0];

    child.stdout.emit("data", "hello\n");
    child.emit("exit", 3, null);

    const events = broadcasts("solverEvent").map(([, , payload]) => payload);
    expect(events).toContainEqual({ kind: "stdout", data: "hello\n" });
    expect(events).toContainEqual({ kind: "exit", code: 3, signal: 0 });
    expect(session.isSolverRunning()).toBe(false);
  });

  it("reports spawn errors as stderr plus an exit", () => {
    session.runSolver();
    spawnedChildren[0].emit("error", new Error("ENOENT"));

    const events = broadcasts("solverEvent").map(([, , payload]) => payload);
    expect(events).toContainEqual({ kind: "exit", code: 1, signal: 0 });
    expect(session.isSolverRunning()).toBe(false);
  });

  it("stops the solver with SIGTERM", () => {
    session.runSolver();
    session.stopSolver();
    expect(spawnedChildren[0].kill).toHaveBeenCalledWith("SIGTERM");
    expect(session.isSolverRunning()).toBe(false);
  });

  it("samples telemetry while running", async () => {
    session.runSolver();
    await vi.waitFor(() => {
      const events = broadcasts("solverEvent").map(([, , payload]) => payload);
      expect(
        events.some(
          (event) =>
            (event as { kind: string }).kind === "sample" &&
            (event as { cpuPercent: number }).cpuPercent === 12,
        ),
      ).toBe(true);
    });
  });
});

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
