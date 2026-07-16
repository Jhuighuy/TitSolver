/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { getDefaultStore } from "jotai";
import {
  afterAll,
  beforeAll,
  beforeEach,
  describe,
  expect,
  it,
  vi,
} from "vitest";

import { type FakeIpc, installFakeIpc } from "~/renderer/common/fake-ipc";
import {
  clearSolverOutput,
  initSolverState,
  isSolverRunningAtom,
  runSolver,
  solverOutputLinesAtom,
  solverSamplesAtom,
  stopSolver,
} from "~/renderer/main/state/solver";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

const store = getDefaultStore();

let runCalls = 0;
let stopCalls = 0;
let failRun = false;
let fake: FakeIpc;

beforeAll(() => {
  fake = installFakeIpc({
    session: {
      isSolverRunning: () => false,
      runSolver: () => {
        runCalls += 1;
        if (failRun) throw new Error("Case has issues.");
      },
      stopSolver: () => {
        stopCalls += 1;
      },
    },
  });
  initSolverState();
});

afterAll(() => {
  fake.uninstall();
});

beforeEach(() => {
  clearSolverOutput();
  store.set(isSolverRunningAtom, false);
  store.set(solverSamplesAtom, []);
  runCalls = 0;
  stopCalls = 0;
  failRun = false;
});

function outputTexts() {
  return store.get(solverOutputLinesAtom).map(({ text }) => text);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

describe("solver state", () => {
  it("splits chunked output into lines", () => {
    fake.emit("session", "solverEvent", { kind: "stdout", data: "a\nb" });
    expect(outputTexts()).toEqual(["a"]);

    // The pending tail joins the next chunk.
    fake.emit("session", "solverEvent", { kind: "stdout", data: "c\nd\n" });
    expect(outputTexts()).toEqual(["a", "bc", "d"]);
  });

  it("tags streams", () => {
    fake.emit("session", "solverEvent", { kind: "stdout", data: "out\n" });
    fake.emit("session", "solverEvent", { kind: "stderr", data: "err\n" });
    expect(
      store.get(solverOutputLinesAtom).map(({ stream, text }) => ({
        stream,
        text,
      })),
    ).toEqual([
      { stream: "stdout", text: "out" },
      { stream: "stderr", text: "err" },
    ]);
  });

  it("collects resource samples", () => {
    fake.emit("session", "solverEvent", {
      kind: "sample",
      timestamp: 123,
      cpuPercent: 42,
      memoryBytes: 1024,
    });
    expect(store.get(solverSamplesAtom)).toHaveLength(1);
    expect(store.get(solverSamplesAtom)[0].cpuPercent).toBe(42);
  });

  it("flushes pending output and resets on exit", () => {
    store.set(isSolverRunningAtom, true);
    fake.emit("session", "solverEvent", { kind: "stderr", data: "tail" });
    expect(outputTexts()).toEqual([]);

    fake.emit("session", "solverEvent", { kind: "exit", code: 0, signal: 0 });
    expect(outputTexts()).toEqual([
      "tail",
      "[Process exited with code 0, signal 0]",
    ]);
    expect(store.get(isSolverRunningAtom)).toBe(false);
  });

  it("runs the solver once and clears the previous output", async () => {
    fake.emit("session", "solverEvent", { kind: "stdout", data: "old\n" });

    runSolver();
    expect(store.get(isSolverRunningAtom)).toBe(true);
    expect(outputTexts()).toEqual([]);
    await vi.waitFor(() => {
      expect(runCalls).toBe(1);
    });

    // Running again while running is a no-op.
    runSolver();
    expect(runCalls).toBe(1);
  });

  it("recovers when the run request fails", async () => {
    failRun = true;
    runSolver();
    await vi.waitFor(() => {
      expect(store.get(isSolverRunningAtom)).toBe(false);
    });
  });

  it("stops only a running solver", async () => {
    stopSolver();
    expect(stopCalls).toBe(0);

    store.set(isSolverRunningAtom, true);
    stopSolver();
    await vi.waitFor(() => {
      expect(stopCalls).toBe(1);
    });
  });
});

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
