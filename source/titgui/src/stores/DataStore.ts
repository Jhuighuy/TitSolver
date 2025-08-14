/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { create } from "zustand";

import { assert } from "~/utils";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export type Field = {
  min: number;
  max: number;
  data: ArrayLike<number>;
};

export type DataSet = Record<string, Field>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

type DataState = {
  numTimeSteps: number;
  setNumTimeSteps: (numTimeSteps: number) => void;

  requestedTimeStep: number | null;
  requestTimeStep: (timeStep: number) => void;
  resolveTimeStep: (timeStep: number, dataset: DataSet) => void;
  isFrameCached: (timeStep: number) => boolean;

  currentTimeStep: number | null;
  currentDataSet: DataSet | null;
  dataSetCache: Map<number, DataSet>;
};

export const useDataStore = create<DataState>((set, get) => ({
  numTimeSteps: 0,
  requestedTimeStep: null,
  timeStepIsReady: false,
  currentTimeStep: null,
  currentDataSet: null,
  dataSetCache: new Map(),

  /** @todo We should handle the case where numTimeSteps increases. */
  setNumTimeSteps(numTimeSteps: number) {
    set({
      numTimeSteps,
      requestedTimeStep: numTimeSteps > 0 ? 0 : null,
      currentTimeStep: null,
      currentDataSet: null,
      dataSetCache: new Map(),
    });
  },

  requestTimeStep(timeStep: number) {
    const { numTimeSteps, dataSetCache: cache } = get();
    assert(timeStep < numTimeSteps);

    // Check the cache first.
    const cachedData = cache.get(timeStep);
    if (cachedData) {
      set({
        requestedTimeStep: null,
        currentTimeStep: timeStep,
        currentDataSet: cachedData,
      });
      return;
    }

    // Actually request the time step.
    set({ requestedTimeStep: timeStep });
  },

  resolveTimeStep(timeStep: number, dataset: DataSet) {
    const { numTimeSteps, requestedTimeStep, dataSetCache: cache } = get();

    // Ignore out-of-bounds time steps. These may occur if the number of time
    // steps changes while the time step is being resolved.
    if (timeStep >= numTimeSteps) return;

    // Update the cache.
    /** @todo We need some strategy for cache eviction. */
    set({ dataSetCache: new Map(cache).set(timeStep, dataset) });

    // Resolve the time step, if this is the requested one.
    if (timeStep !== requestedTimeStep) return;
    set({
      requestedTimeStep: null,
      currentTimeStep: timeStep,
      currentDataSet: dataset,
    });
  },

  isFrameCached(timeStep: number) {
    const { numTimeSteps, dataSetCache: cache } = get();
    assert(timeStep < numTimeSteps);

    return cache.has(timeStep);
  },
}));

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
