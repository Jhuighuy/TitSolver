/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { atom, getDefaultStore } from "jotai";

import { ipc } from "~/renderer/common/ipc";
import { logger } from "~/renderer/common/logging";
import { FieldMap } from "~/renderer/common/visual/fields";
import { frameDataAtom } from "~/renderer/main/state/viewport";
import { assert } from "~/shared/utils";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/** Number of frames in the open series (`0` while unknown or empty). */
export const numFramesAtom = atom(0);

/** Index of the displayed frame, or `null` if none is loaded yet. */
export const frameIndexAtom = atom<number | null>(null);

// Concurrency guards: responses are dropped unless they belong to the latest
// refresh (session) and the latest frame request.
let sessionID = 0;
let requestID = 0;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
 * Reload the frame count and re-request the displayed frame, keeping the
 * current position when possible. Also used for the initial load.
 */
export function refreshStorage() {
  const store = getDefaultStore();
  const session = ++sessionID;
  requestID++;

  void ipc.session
    .frameCount()
    .then((numFrames) => {
      if (session !== sessionID) return;

      store.set(numFramesAtom, numFrames);
      if (numFrames === 0) {
        store.set(frameIndexAtom, null);
        store.set(frameDataAtom, new FieldMap({}));
        return;
      }

      // Stay on the current frame, clamped to the new frame count.
      const frameIndex = Math.min(
        store.get(frameIndexAtom) ?? 0,
        numFrames - 1,
      );
      store.set(frameIndexAtom, frameIndex);
      void requestFrame(frameIndex);
    })
    .catch((error: unknown) => {
      logger.err("Failed to query the frame count.\n", error);
    });
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

let initialized = false;

/**
 * Load the initial storage state and refresh whenever the storage changes on
 * disk. Idempotent; the subscription lives for the lifetime of the window.
 */
export function initStorageState() {
  if (initialized) return;
  initialized = true;

  ipc.session.onStorageChanged(() => {
    logger.log("Storage changed on disk; refreshing.");
    refreshStorage();
  });
  refreshStorage();
}

/**
 * Load the frame with the given index and display it. The returned promise
 * settles once the frame is applied (or superseded by a newer request).
 */
export async function requestFrame(frameIndex: number) {
  const store = getDefaultStore();
  const numFrames = store.get(numFramesAtom);
  assert(0 <= frameIndex && frameIndex < numFrames);

  const session = sessionID;
  const request = ++requestID;

  try {
    const result = await ipc.session.frame(frameIndex);
    if (session !== sessionID || request !== requestID) return;

    const fieldMap = new FieldMap(
      Object.fromEntries(
        Object.entries(result).map(([fieldName, { data }]) => [
          fieldName,
          data,
        ]),
      ),
    );

    store.set(frameDataAtom, fieldMap);
    store.set(frameIndexAtom, frameIndex);
  } catch (error) {
    logger.err("Failed to read the frame.\n", error);
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
