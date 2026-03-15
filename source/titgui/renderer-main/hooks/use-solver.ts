/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import {
  useQuery,
  useQueryClient,
  type QueryClient,
} from "@tanstack/react-query";
import { useCallback, useEffect } from "react";

import { downloadText } from "~/renderer-common/utils";
import { solverEventSchema, type SolverSample } from "~/shared/solver";
import { assert } from "~/shared/utils";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

const solverStateQueryKey = ["solver", "state"] as const;
const elapsedIntervalMs = 1000;
const maxOutputLines = 500;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export interface SolverOutputLine {
  id: number;
  stream: "stdout" | "stderr";
  text: string;
  timestamp: number;
}

interface SolverState {
  isRunning: boolean;
  elapsedMs: number;
  runStartedAt: number | null;
  samples: SolverSample[];
  outputLines: SolverOutputLine[];
  partialOutput: Record<"stdout" | "stderr", string>;
  nextOutputLineID: number;
}

export interface Solver {
  isSolverRunning: boolean;
  elapsedMs: number;
  samples: SolverSample[];
  outputLines: SolverOutputLine[];
  runSolver: () => void;
  stopSolver: () => void;
  clearSolverOutput: () => void;
  downloadSolverOutput: () => void;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

const initialSolverState = (): SolverState => ({
  isRunning: false,
  elapsedMs: 0,
  runStartedAt: null,
  samples: [],
  outputLines: [],
  partialOutput: {
    stdout: "",
    stderr: "",
  },
  nextOutputLineID: 0,
});

let isSubscribed = false;
let elapsedTimer: ReturnType<typeof globalThis.setInterval> | undefined;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

function getSolverState(queryClient: QueryClient): SolverState {
  return (
    queryClient.getQueryData<SolverState>(solverStateQueryKey) ??
    initialSolverState()
  );
}

function setSolverState(
  queryClient: QueryClient,
  update: (state: SolverState) => SolverState,
) {
  queryClient.setQueryData<SolverState>(solverStateQueryKey, (state) =>
    update(state ?? initialSolverState()),
  );
}

function stopElapsedTimer() {
  if (elapsedTimer === undefined) return;
  globalThis.clearInterval(elapsedTimer);
  elapsedTimer = undefined;
}

function ensureElapsedTimer(queryClient: QueryClient) {
  if (elapsedTimer !== undefined) return;

  elapsedTimer = globalThis.setInterval(() => {
    const state = getSolverState(queryClient);
    if (!state.isRunning || state.runStartedAt === null) return;

    setSolverState(queryClient, (currentState) => ({
      ...currentState,
      elapsedMs: Date.now() - (currentState.runStartedAt ?? Date.now()),
    }));
  }, elapsedIntervalMs);
}

function appendOutputChunk(
  state: SolverState,
  stream: "stdout" | "stderr",
  chunk: string,
  timestamp: number,
): SolverState {
  const buffer = state.partialOutput[stream] + chunk;
  const parts = buffer.split("\n");
  const trailing = parts.pop() ?? "";

  if (parts.length === 0) {
    return {
      ...state,
      partialOutput: {
        ...state.partialOutput,
        [stream]: trailing,
      },
    };
  }

  const nextLines = parts.map((text, index) => ({
    id: state.nextOutputLineID + index,
    stream,
    text,
    timestamp,
  }));

  const outputLines = [...state.outputLines, ...nextLines].slice(
    -maxOutputLines,
  );

  return {
    ...state,
    outputLines,
    partialOutput: {
      ...state.partialOutput,
      [stream]: trailing,
    },
    nextOutputLineID: state.nextOutputLineID + nextLines.length,
  };
}

function flushPartialOutput(
  state: SolverState,
  timestamp: number,
): SolverState {
  let nextState = state;
  for (const stream of ["stdout", "stderr"] as const) {
    const text = nextState.partialOutput[stream];
    if (text.length === 0) continue;

    const line: SolverOutputLine = {
      id: nextState.nextOutputLineID,
      stream,
      text,
      timestamp,
    };

    nextState = {
      ...nextState,
      outputLines: [...nextState.outputLines, line].slice(-maxOutputLines),
      partialOutput: {
        ...nextState.partialOutput,
        [stream]: "",
      },
      nextOutputLineID: nextState.nextOutputLineID + 1,
    };
  }

  return nextState;
}

function solverOutputText(state: SolverState): string {
  const lines = state.outputLines.map(({ text }) => text);
  for (const stream of ["stdout", "stderr"] as const) {
    const text = state.partialOutput[stream];
    if (text.length > 0) lines.push(text);
  }
  return lines.join("\n");
}

function subscribeSolver(queryClient: QueryClient) {
  if (isSubscribed) return;
  isSubscribed = true;

  void globalThis.session?.isSolverRunning().then((isRunning) => {
    setSolverState(queryClient, (state) => ({
      ...state,
      isRunning,
      runStartedAt: isRunning ? Date.now() : null,
      elapsedMs: 0,
    }));

    if (isRunning) {
      ensureElapsedTimer(queryClient);
    } else {
      stopElapsedTimer();
    }
  });

  globalThis.session?.onSolverEvent((rawEvent) => {
    const event = solverEventSchema.parse(rawEvent);
    const timestamp = Date.now();

    setSolverState(queryClient, (state) => {
      switch (event.kind) {
        case "stdout":
        case "stderr":
          return appendOutputChunk(state, event.kind, event.data, timestamp);

        case "sample":
          return {
            ...state,
            samples:
              state.samples.at(-1)?.timestamp === event.timestamp
                ? state.samples
                : [...state.samples, event],
          };

        case "exit": {
          const flushedState = flushPartialOutput(state, timestamp);
          const exitLine: SolverOutputLine = {
            id: flushedState.nextOutputLineID,
            stream: "stderr",
            text: `[Process exited with code ${event.code}, signal ${event.signal}]`,
            timestamp,
          };

          return {
            ...flushedState,
            isRunning: false,
            elapsedMs:
              flushedState.runStartedAt === null
                ? flushedState.elapsedMs
                : Date.now() - flushedState.runStartedAt,
            runStartedAt: null,
            outputLines: [...flushedState.outputLines, exitLine].slice(
              -maxOutputLines,
            ),
            nextOutputLineID: flushedState.nextOutputLineID + 1,
          };
        }
      }
    });

    if (event.kind === "exit") {
      stopElapsedTimer();
    } else {
      ensureElapsedTimer(queryClient);
    }
  });
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export function useSolver(): Solver {
  const queryClient = useQueryClient();

  const { data: state } = useQuery({
    queryKey: solverStateQueryKey,
    queryFn: () => initialSolverState(),
    initialData: initialSolverState,
    staleTime: Number.POSITIVE_INFINITY,
    gcTime: Number.POSITIVE_INFINITY,
  });

  useEffect(() => {
    subscribeSolver(queryClient);
  }, [queryClient]);

  const runSolver = useCallback(() => {
    const state = getSolverState(queryClient);
    assert(!state.isRunning);

    const runStartedAt = Date.now();
    setSolverState(queryClient, () => ({
      ...initialSolverState(),
      isRunning: true,
      runStartedAt,
    }));
    ensureElapsedTimer(queryClient);
    void globalThis.session?.runSolver();
  }, [queryClient]);

  const stopSolver = useCallback(() => {
    assert(getSolverState(queryClient).isRunning);
    void globalThis.session?.stopSolver();
  }, [queryClient]);

  const clearSolverOutput = useCallback(() => {
    setSolverState(queryClient, (state) => ({
      ...state,
      outputLines: [],
      partialOutput: {
        stdout: "",
        stderr: "",
      },
    }));
  }, [queryClient]);

  const downloadSolverOutput = useCallback(() => {
    downloadText(
      "solver-output.txt",
      solverOutputText(getSolverState(queryClient)),
    );
  }, [queryClient]);

  return {
    isSolverRunning: state.isRunning,
    elapsedMs: state.elapsedMs,
    samples: state.samples,
    outputLines: state.outputLines,
    runSolver,
    stopSolver,
    clearSolverOutput,
    downloadSolverOutput,
  };
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
