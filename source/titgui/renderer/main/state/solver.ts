/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { atom, getDefaultStore } from "jotai";

import { ipc } from "~/renderer/common/ipc";
import { LineBuffer } from "~/renderer/common/line-buffer";
import { logger } from "~/renderer/common/logging";
import { downloadText } from "~/renderer/common/utils";
import type { SolverSample } from "~/shared/solver";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export type SolverOutputStream = "stdout" | "stderr";

export interface SolverOutputLine {
  id: number;
  stream: SolverOutputStream;
  text: string;
  timestamp: number;
}

export const isSolverRunningAtom = atom(false);

/** Recent solver output, split into lines (bounded). */
export const solverOutputLinesAtom = atom<readonly SolverOutputLine[]>([]);

/** Resource usage samples of the current run (bounded). */
export const solverSamplesAtom = atom<readonly SolverSample[]>([]);

/** Wall-clock duration of the current (or last finished) run. */
export const solverElapsedMsAtom = atom(0);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

const MAX_OUTPUT_LINES = 500;
const MAX_SAMPLES = 4096;
const ELAPSED_INTERVAL_MS = 1000;

let lineBuffers = {
  stdout: new LineBuffer(),
  stderr: new LineBuffer(),
};
let nextLineID = 0;
let runStartedAt: number | null = null;
let elapsedTimer: NodeJS.Timeout | undefined;

// Append complete output lines, keeping the log bounded.
function appendOutputLines(
  stream: SolverOutputStream,
  texts: readonly string[],
  timestamp: number,
) {
  if (texts.length === 0) return;
  const store = getDefaultStore();
  const lines = texts.map((text) => ({
    id: nextLineID++,
    stream,
    text,
    timestamp,
  }));
  store.set(
    solverOutputLinesAtom,
    [...store.get(solverOutputLinesAtom), ...lines].slice(-MAX_OUTPUT_LINES),
  );
}

function startElapsedTimer() {
  stopElapsedTimer();
  elapsedTimer = setInterval(() => {
    if (runStartedAt === null) return;
    getDefaultStore().set(solverElapsedMsAtom, Date.now() - runStartedAt);
  }, ELAPSED_INTERVAL_MS);
}

function stopElapsedTimer() {
  if (elapsedTimer === undefined) return;
  clearInterval(elapsedTimer);
  elapsedTimer = undefined;
}

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
      if (running) {
        // The actual start time is unknown after a window reload; measure
        // the elapsed time from now on.
        runStartedAt = Date.now();
        startElapsedTimer();
      }
    })
    .catch((error: unknown) => {
      logger.err("Failed to query the solver state.\n", error);
    });

  ipc.session.onSolverEvent((event) => {
    const timestamp = Date.now();
    switch (event.kind) {
      case "stdout":
      case "stderr":
        appendOutputLines(
          event.kind,
          lineBuffers[event.kind].append(event.data),
          timestamp,
        );
        break;

      case "sample":
        store.set(
          solverSamplesAtom,
          [...store.get(solverSamplesAtom), event].slice(-MAX_SAMPLES),
        );
        break;

      case "exit":
        for (const stream of ["stdout", "stderr"] as const) {
          appendOutputLines(stream, lineBuffers[stream].flush(), timestamp);
        }
        appendOutputLines(
          "stderr",
          [`[Process exited with code ${event.code}, signal ${event.signal}]`],
          timestamp,
        );
        if (runStartedAt !== null) {
          store.set(solverElapsedMsAtom, timestamp - runStartedAt);
          runStartedAt = null;
        }
        stopElapsedTimer();
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

  clearSolverOutput();
  store.set(solverSamplesAtom, []);
  store.set(solverElapsedMsAtom, 0);
  runStartedAt = Date.now();
  startElapsedTimer();

  store.set(isSolverRunningAtom, true);
  void ipc.session.runSolver().catch((error: unknown) => {
    store.set(isSolverRunningAtom, false);
    runStartedAt = null;
    stopElapsedTimer();
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

/**
 * Clear the solver output.
 */
export function clearSolverOutput() {
  lineBuffers = { stdout: new LineBuffer(), stderr: new LineBuffer() };
  getDefaultStore().set(solverOutputLinesAtom, []);
}

/**
 * Download the solver output as a text file.
 */
export function downloadSolverOutput() {
  const store = getDefaultStore();
  const lines = store.get(solverOutputLinesAtom).map(({ text }) => text);
  for (const stream of ["stdout", "stderr"] as const) {
    const pending = lineBuffers[stream].pending;
    if (pending.length > 0) lines.push(pending);
  }
  downloadText("solver-output.txt", lines.join("\n"));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
