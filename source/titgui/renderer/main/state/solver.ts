/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { atom, getDefaultStore } from "jotai";

import { ipc } from "~/renderer/common/ipc";
import { logger } from "~/renderer/common/logging";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export const isSolverRunningAtom = atom(false);
export const solverOutputAtom = atom("");

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

let initialized = false;

/**
 * Query the initial solver state and subscribe to solver events. Idempotent;
 * the subscription lives for the lifetime of the window.
 */
export function initSolverState() {
  if (initialized) return;
  initialized = true;

  const store = getDefaultStore();

  void ipc.session
    .isSolverRunning()
    .then((running) => {
      store.set(isSolverRunningAtom, running);
    })
    .catch((error: unknown) => {
      logger.err("Failed to query the solver state.\n", error);
    });

  ipc.session.onSolverEvent((event) => {
    switch (event.kind) {
      case "stdout":
      case "stderr":
        store.set(solverOutputAtom, store.get(solverOutputAtom) + event.data);
        break;

      case "exit":
        store.set(
          solverOutputAtom,
          `${store.get(solverOutputAtom)}\n[Process exited with code ${event.code}, signal ${event.signal}]\n`,
        );
        store.set(isSolverRunningAtom, false);
        break;
    }
  });
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
 * Run the solver. No-op if it is already running.
 */
export function runSolver() {
  const store = getDefaultStore();
  if (store.get(isSolverRunningAtom)) return;

  store.set(isSolverRunningAtom, true);
  store.set(solverOutputAtom, "");
  void ipc.session.runSolver().catch((error: unknown) => {
    store.set(isSolverRunningAtom, false);
    logger.err("Failed to run the solver.\n", error);
  });
}

/**
 * Stop the solver. No-op if it is not running.
 */
export function stopSolver() {
  const store = getDefaultStore();
  if (!store.get(isSolverRunningAtom)) return;

  void ipc.session.stopSolver().catch((error: unknown) => {
    logger.err("Failed to stop the solver.\n", error);
  });
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
