/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { z } from "zod";

import {
  caseDocumentSchema,
  caseStateSchema,
  recentCaseSchema,
  specJsonSchema,
  treeJsonSchema,
} from "~/shared/case";
import { helpSessionSchema } from "~/shared/help";
import { event, type IpcClientOf, method, service } from "~/shared/ipc/core";
import { solverEventSchema } from "~/shared/solver";
import { frameSchema } from "~/shared/storage";
import { themeSchema } from "~/shared/theme";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
 * The IPC contract: the single source of truth for everything that crosses
 * the main/renderer boundary. Channel names, argument and result validation,
 * the preload client, and the main-process handler types are all derived
 * from this object.
 */
export const ipcContract = {
  theme: service({
    methods: {
      get: method({ result: themeSchema }),
      set: method({ args: [themeSchema], result: z.void() }),
    },
    events: {},
  }),

  window: service({
    methods: {
      persistGet: method({ args: [z.string()], result: z.unknown() }),
      persistSet: method({
        args: [z.string(), z.unknown()],
        result: z.void(),
      }),
      isFullScreen: method({ result: z.boolean() }),
    },
    events: {
      fullScreenChanged: event(z.boolean()),
    },
  }),

  case: service({
    methods: {
      state: method({ result: caseStateSchema }),
      recents: method({ result: z.array(recentCaseSchema) }),
      // `newCase` and `openCase` show a directory dialog in the main
      // process; both resolve to `null` when the dialog is cancelled.
      newCase: method({ result: caseStateSchema }),
      openCase: method({ result: caseStateSchema }),
      openRecent: method({ args: [z.string()], result: caseStateSchema }),
      save: method({ result: z.void() }),
      close: method({ result: z.void() }),
      getSpec: method({ result: specJsonSchema }),
      document: method({ result: caseDocumentSchema.nullable() }),
      // Edits carry the document revision they were based on; a stale
      // revision is rejected (`false`) and must be retried on top of the
      // latest document.
      updateTree: method({
        args: [treeJsonSchema, z.int().nonnegative()],
        result: z.boolean(),
      }),
    },
    events: {
      caseChanged: event(caseStateSchema),
      treeChanged: event(caseDocumentSchema),
    },
  }),

  session: service({
    methods: {
      frameCount: method({ result: z.int().nonnegative() }),
      frame: method({ args: [z.int().nonnegative()], result: frameSchema }),
      export: method({ result: z.void() }),
      runSolver: method({ result: z.void() }),
      stopSolver: method({ result: z.void() }),
      isSolverRunning: method({ result: z.boolean() }),
    },
    events: {
      solverEvent: event(solverEventSchema),
      storageChanged: event(z.null()),
    },
  }),

  help: service({
    methods: {
      getSession: method({ result: helpSessionSchema }),
      addTab: method({ args: [z.string().optional()], result: z.void() }),
      closeTab: method({ args: [z.number()], result: z.void() }),
      selectTab: method({ args: [z.number()], result: z.void() }),
      navigateTab: method({
        args: [z.number(), z.string().optional()],
        result: z.void(),
      }),
    },
    events: {
      sessionChanged: event(helpSessionSchema),
    },
  }),
} as const;

/**
 * The IPC contract type.
 */
export type IpcContract = typeof ipcContract;

/**
 * Client-side API of the IPC contract, exposed to the renderer as the
 * `titgui` global.
 */
export type IpcClient = IpcClientOf<IpcContract>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
