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
  isPlayingAtom,
  isRepeatingAtom,
  startPlayback,
  stopPlayback,
  togglePlayback,
} from "~/renderer/main/state/playback";
import {
  frameIndexAtom,
  numFramesAtom,
  refreshStorage,
  requestFrame,
} from "~/renderer/main/state/storage";
import type { Frame } from "~/shared/storage";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

const store = getDefaultStore();

const NUM_FRAMES = 3;

function makeFrame(): Frame {
  return {
    rho: {
      type: { kind: "float32_t", rank: 0, dim: 2 },
      data: new Float32Array([0]),
    },
    r: {
      type: { kind: "float32_t", rank: 1, dim: 2 },
      data: new Float32Array([0, 0]),
    },
  };
}

let fake: FakeIpc;
let frameReads = 0;

beforeAll(async () => {
  fake = installFakeIpc({
    session: {
      frameCount: () => NUM_FRAMES,
      frameTimes: () => [0, 1, 2],
      frame: () => {
        frameReads += 1;
        return makeFrame();
      },
    },
  });

  // Wait until the refresh fully lands, including its own follow-up frame
  // request — a late response would race the tests' navigation.
  refreshStorage();
  await vi.waitFor(() => {
    expect(store.get(numFramesAtom)).toBe(NUM_FRAMES);
    expect(frameReads).toBeGreaterThan(0);
  });
});

afterAll(() => {
  fake.uninstall();
});

beforeEach(async () => {
  stopPlayback();
  store.set(isRepeatingAtom, false);
  await requestFrame(0);
});

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

describe("playback", () => {
  it("plays to the last frame and stops", async () => {
    startPlayback();
    expect(store.get(isPlayingAtom)).toBe(true);

    await vi.waitFor(() => {
      expect(store.get(frameIndexAtom)).toBe(NUM_FRAMES - 1);
      expect(store.get(isPlayingAtom)).toBe(false);
    });
  });

  it("wraps around when repeating", async () => {
    store.set(isRepeatingAtom, true);
    await requestFrame(NUM_FRAMES - 1);

    // Record the frames the loop visits; polling the atom could miss the
    // brief moment a wrapped-around frame is displayed.
    const visited = new Set<number | null>();
    const unsubscribe = store.sub(frameIndexAtom, () => {
      visited.add(store.get(frameIndexAtom));
    });

    startPlayback();
    await vi.waitFor(() => {
      expect(visited.has(0)).toBe(true);
      expect(visited.has(1)).toBe(true);
    });
    unsubscribe();

    stopPlayback();
    expect(store.get(isPlayingAtom)).toBe(false);
  });

  it("stops advancing when cancelled", async () => {
    startPlayback();
    stopPlayback();
    expect(store.get(isPlayingAtom)).toBe(false);

    // The frame request in flight at cancellation time may still land;
    // after that, the position must not move again.
    await sleep(100);
    const settled = store.get(frameIndexAtom);
    await sleep(100);
    expect(store.get(frameIndexAtom)).toBe(settled);
  });

  it("toggles", () => {
    togglePlayback();
    expect(store.get(isPlayingAtom)).toBe(true);
    togglePlayback();
    expect(store.get(isPlayingAtom)).toBe(false);
  });
});

async function sleep(durationMs: number) {
  return new Promise<void>((resolve) => {
    setTimeout(resolve, durationMs);
  });
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
