/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { atom, getDefaultStore } from "jotai";

import { ipc } from "~/renderer/common/ipc";
import { logger } from "~/renderer/common/logging";
import {
  type CaseDocument,
  type CaseState,
  type RecentCase,
  type SpecJson,
  treeDeleteAt,
  type TreeJson,
  treeSetAt,
} from "~/shared/case";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/** State of the open case, or `null` when no case is open. */
export const caseStateAtom = atom<CaseState>(null);

/** Recently opened cases, most recent first. */
export const recentCasesAtom = atom<RecentCase[]>([]);

/** The case specification; `null` until loaded. */
export const caseSpecAtom = atom<SpecJson | null>(null);

/** The current case document, or `null` when no case is open. */
export const caseDocumentAtom = atom<CaseDocument | null>(null);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

let initialized = false;

/**
 * Load the initial case state and keep it in sync with the main process.
 * Idempotent; the subscription lives for the lifetime of the window.
 */
export function initCaseState() {
  if (initialized) return;
  initialized = true;

  ipc.case.onCaseChanged((state) => {
    getDefaultStore().set(caseStateAtom, state);
    if (state === null) getDefaultStore().set(caseDocumentAtom, null);
    refreshRecents();
  });

  // Documents can arrive out of order while edits are in flight; keep the
  // highest revision. A document for a different case always follows a
  // `caseChanged` reset, so revision comparison stays within one case.
  ipc.case.onTreeChanged((document) => {
    const store = getDefaultStore();
    const current = store.get(caseDocumentAtom);
    if (current === null || document.revision >= current.revision) {
      store.set(caseDocumentAtom, document);
    }
  });

  void ipc.case
    .state()
    .then((state) => {
      getDefaultStore().set(caseStateAtom, state);
    })
    .catch((error: unknown) => {
      logger.err("Failed to query the case state.\n", error);
    });
  void ipc.case
    .getSpec()
    .then((spec) => {
      getDefaultStore().set(caseSpecAtom, spec);
    })
    .catch((error: unknown) => {
      logger.err("Failed to query the case specification.\n", error);
    });
  refreshDocument();
  refreshRecents();
}

// Reload the current case document.
function refreshDocument() {
  void ipc.case
    .document()
    .then((document) => {
      getDefaultStore().set(caseDocumentAtom, document);
    })
    .catch((error: unknown) => {
      logger.err("Failed to query the case document.\n", error);
    });
}

// Reload the recents list.
function refreshRecents() {
  void ipc.case
    .recents()
    .then((recents) => {
      getDefaultStore().set(recentCasesAtom, recents);
    })
    .catch((error: unknown) => {
      logger.err("Failed to query the recent cases.\n", error);
    });
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
 * Create a new case via a directory dialog. Resolves to the new case state,
 * or `null` when the dialog was cancelled.
 */
export async function newCase() {
  try {
    return await ipc.case.newCase();
  } catch (error) {
    logger.err("Failed to create the case.\n", error);
    return null;
  }
}

/**
 * Open a case via a directory dialog. Resolves to the opened case state, or
 * `null` when the dialog was cancelled.
 */
export async function openCase() {
  try {
    return await ipc.case.openCase();
  } catch (error) {
    logger.err("Failed to open the case.\n", error);
    return null;
  }
}

/**
 * Open a recently used case. Resolves to the opened case state, or `null`
 * on failure.
 */
export async function openRecentCase(dir: string) {
  try {
    return await ipc.case.openRecent(dir);
  } catch (error) {
    logger.err("Failed to open the case.\n", error);
    return null;
  }
}

/**
 * Save the case file.
 */
export async function saveCase() {
  try {
    await ipc.case.save();
  } catch (error) {
    logger.err("Failed to save the case.\n", error);
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Submit an edited authored tree. On a stale revision the edit is dropped
// and the latest document is refetched — the next edit lands on top of it.
async function submitAuthoredTree(authored: TreeJson, revision: number) {
  try {
    const accepted = await ipc.case.updateTree(authored, revision);
    if (!accepted) refreshDocument();
  } catch (error) {
    logger.err("Failed to update the case.\n", error);
  }
}

/**
 * Set an authored value at the given tree path.
 */
export function setCaseValue(path: readonly string[], value: TreeJson) {
  const document = getDefaultStore().get(caseDocumentAtom);
  if (document === null) return;
  void submitAuthoredTree(
    treeSetAt(document.authored, path, value),
    document.revision,
  );
}

/**
 * Reset the value at the given tree path to its default by deleting the
 * authored node.
 */
export function resetCaseValue(path: readonly string[]) {
  const document = getDefaultStore().get(caseDocumentAtom);
  if (document === null) return;
  void submitAuthoredTree(
    treeDeleteAt(document.authored, path),
    document.revision,
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
