/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { atom, getDefaultStore } from "jotai";

import {
  frameIndexAtom,
  numFramesAtom,
  requestFrame,
} from "~/renderer/main/state/storage";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export const isPlayingAtom = atom(false);
export const isRepeatingAtom = atom(false);

// Target pacing; playback never outruns frame delivery, since the next frame
// is requested only after the current one is applied.
const FRAME_INTERVAL_MS = 1000 / 60;

// Bumping the token cancels the running playback loop.
let playbackToken = 0;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
 * Start the playback. No-op if it is already running.
 */
export function startPlayback() {
  const store = getDefaultStore();
  if (store.get(isPlayingAtom)) return;

  store.set(isPlayingAtom, true);
  void playbackLoop(++playbackToken);
}

/**
 * Stop the playback. No-op if it is not running.
 */
export function stopPlayback() {
  playbackToken++;
  getDefaultStore().set(isPlayingAtom, false);
}

/**
 * Toggle the playback.
 */
export function togglePlayback() {
  if (getDefaultStore().get(isPlayingAtom)) stopPlayback();
  else startPlayback();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Request-driven playback: apply a frame, then schedule the next one.
async function playbackLoop(token: number) {
  const store = getDefaultStore();

  // The module-level token moves when this loop is cancelled; it must be
  // re-checked after every await.
  const isCancelled = () => token !== playbackToken;

  while (!isCancelled()) {
    const numFrames = store.get(numFramesAtom);
    const frameIndex = store.get(frameIndexAtom);
    if (numFrames === 0 || frameIndex === null) break;
    if (!store.get(isRepeatingAtom) && frameIndex === numFrames - 1) break;

    /* oxlint-disable no-await-in-loop -- paced by frame delivery. */
    const startTime = performance.now();
    await requestFrame((frameIndex + 1) % numFrames);
    const elapsed = performance.now() - startTime;
    await sleep(Math.max(0, FRAME_INTERVAL_MS - elapsed));
    /* oxlint-enable no-await-in-loop */
  }

  if (!isCancelled()) stopPlayback();
}

async function sleep(durationMs: number) {
  return new Promise<void>((resolve) => {
    setTimeout(resolve, durationMs);
  });
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
