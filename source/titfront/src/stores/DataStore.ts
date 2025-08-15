/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { create } from "zustand";

import { assert } from "~/utils";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export type DataSet = Record<string, ArrayLike<number>>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

type DataState = {
  numTimeSteps: number;
  setNumTimeSteps: (numTimeSteps: number) => void;

  requestedTimeStep: number | null;
  requestTimeStep: (timeStep: number) => void;
  resolveTimeStep: (timeStep: number, dataset: DataSet) => void;
  isFrameCached: (timeStep: number) => boolean;

  currentTimeStep: number | null;
  currentTimeStepData: DataSet | null;

  cache: Map<number, DataSet>;
};

export const useDataStore = create<DataState>((set, get) => ({
  numTimeSteps: 0,
  requestedTimeStep: null,
  timeStepIsReady: false,
  currentTimeStep: null,
  currentTimeStepData: null,
  cache: new Map(),

  /** @todo We should handle the case where numTimeSteps increases. */
  setNumTimeSteps(numTimeSteps: number) {
    set({
      numTimeSteps,
      requestedTimeStep: numTimeSteps > 0 ? 0 : null,
      currentTimeStep: null,
      currentTimeStepData: null,
      cache: new Map(),
    });
  },

  requestTimeStep(timeStep: number) {
    const { numTimeSteps, cache } = get();
    assert(timeStep < numTimeSteps);

    // Check the cache first.
    if (cache.has(timeStep)) {
      set({
        requestedTimeStep: null,
        currentTimeStep: timeStep,
        currentTimeStepData: cache.get(timeStep),
      });
      return;
    }

    // Actually request the time step.
    set({ requestedTimeStep: timeStep });
  },

  resolveTimeStep(timeStep: number, dataset: DataSet) {
    const { numTimeSteps, requestedTimeStep, cache } = get();

    // Ignore out-of-bounds time steps. These may occur if the number of time
    // steps changes while the time step is being resolved.
    if (timeStep >= numTimeSteps) return;

    // Update the cache.
    /** @todo We need some strategy for cache eviction. */
    set({ cache: new Map(cache).set(timeStep, dataset) });

    // Resolve the time step, if this is the requested one.
    if (timeStep !== requestedTimeStep) return;
    set({
      requestedTimeStep: null,
      currentTimeStep: timeStep,
      currentTimeStepData: dataset,
    });
  },

  isFrameCached(timeStep: number) {
    const { numTimeSteps, cache } = get();
    assert(timeStep < numTimeSteps);

    return cache.has(timeStep);
  },
}));

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
