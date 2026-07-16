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
import { FieldMap } from "~/renderer/common/visual/fields";
import {
  frameIndexAtom,
  frameTimeAtom,
  frameTimesAtom,
  initStorageState,
  numFramesAtom,
  refreshStorage,
  requestFrame,
} from "~/renderer/main/state/storage";
import { frameDataAtom } from "~/renderer/main/state/viewport";
import type { Frame } from "~/shared/storage";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

const store = getDefaultStore();

// A one-particle frame whose density value identifies it.
function makeFrame(value: number): Frame {
  return {
    rho: {
      type: { kind: "float32_t", rank: 0, dim: 2 },
      data: new Float32Array([value]),
    },
    r: {
      type: { kind: "float32_t", rank: 1, dim: 2 },
      data: new Float32Array([0, 0]),
    },
  };
}

// The fake backend: a mutable frame list, optionally answering frame reads
// with manually-resolved promises to exercise the stale-response guard.
let frames: Frame[] = [];
let deferred: Map<number, (frame: Frame) => void> | null = null;
let fake: FakeIpc;

beforeAll(() => {
  fake = installFakeIpc({
    session: {
      frameCount: () => frames.length,
      frameTimes: () => frames.map((_frame, index) => index / 2),
      frame: async (_context, index) => {
        if (deferred !== null) {
          return new Promise<Frame>((resolve) => {
            deferred?.set(index, resolve);
          });
        }
        return frames[index];
      },
    },
  });
  initStorageState();
});

afterAll(() => {
  fake.uninstall();
});

// Decouple tests from the position the previous test navigated to: the
// next refresh then always lands on frame 0.
beforeEach(() => {
  store.set(numFramesAtom, 0);
  store.set(frameIndexAtom, null);
  store.set(frameTimesAtom, []);
  store.set(frameDataAtom, new FieldMap({}));
});

function displayedDensity() {
  return store.get(frameDataAtom).get("rho").data[0];
}

// Refresh and wait until the refresh fully lands, including its own
// follow-up frame request — otherwise that late response races the test's
// navigation. The expected density identifies the frame the refresh loads.
async function refreshAndSettle(
  expectedFrames: number,
  expectedDensity?: number,
) {
  refreshStorage();
  await vi.waitFor(() => {
    expect(store.get(numFramesAtom)).toBe(expectedFrames);
    if (expectedDensity !== undefined) {
      expect(displayedDensity()).toBe(expectedDensity);
    }
  });
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

describe("storage state", () => {
  it("loads the frame count, times, and first frame on refresh", async () => {
    frames = [makeFrame(1), makeFrame(2), makeFrame(3)];
    await refreshAndSettle(3, 1);

    expect(store.get(frameIndexAtom)).toBe(0);
    expect(store.get(frameTimesAtom)).toEqual([0, 0.5, 1]);
  });

  it("derives the physical time of the displayed frame", async () => {
    frames = [makeFrame(11), makeFrame(12), makeFrame(13)];
    await refreshAndSettle(3, 11);
    await requestFrame(2);

    expect(store.get(frameTimeAtom)).toBe(1);
  });

  it("keeps the current frame across refreshes, clamped", async () => {
    frames = [makeFrame(21), makeFrame(22), makeFrame(23)];
    await refreshAndSettle(3, 21);
    await requestFrame(2);

    frames = [makeFrame(21), makeFrame(22)];
    await refreshAndSettle(2, 22);
    expect(store.get(frameIndexAtom)).toBe(1);
  });

  it("clears everything when the storage empties", async () => {
    frames = [makeFrame(31)];
    await refreshAndSettle(1, 31);

    frames = [];
    await refreshAndSettle(0);
    expect(store.get(frameIndexAtom)).toBeNull();
    expect(store.get(frameTimesAtom)).toEqual([]);
    // The empty field map synthesizes zero-length placeholder fields.
    expect(store.get(frameDataAtom).count).toBe(0);
    expect(store.get(frameTimeAtom)).toBeNull();
  });

  it("refreshes when the storage changes on disk", async () => {
    frames = [];
    await refreshAndSettle(0);

    frames = [makeFrame(7)];
    fake.emit("session", "storageChanged", null);
    await vi.waitFor(() => {
      expect(store.get(numFramesAtom)).toBe(1);
      expect(displayedDensity()).toBe(7);
    });
  });

  it("drops stale frame responses", async () => {
    frames = [makeFrame(41), makeFrame(42)];
    await refreshAndSettle(2, 41);

    // Two in-flight requests; the older one resolves last and must lose.
    deferred = new Map();
    const first = requestFrame(0);
    const second = requestFrame(1);
    await vi.waitFor(() => {
      expect(deferred?.size).toBe(2);
    });

    deferred.get(1)?.(makeFrame(20));
    await second;
    expect(store.get(frameIndexAtom)).toBe(1);
    expect(displayedDensity()).toBe(20);

    deferred.get(0)?.(makeFrame(10));
    await first;
    expect(store.get(frameIndexAtom)).toBe(1);
    expect(displayedDensity()).toBe(20);

    deferred = null;
  });
});

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
