/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import {
  type BrowserWindow,
  dialog,
  type MessageBoxOptions,
  type OpenDialogOptions,
  type SaveDialogOptions,
} from "electron";

import type { CaseManager } from "~/main/case";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
 * User-facing case workflows: the directory dialogs and the unsaved-changes
 * guard around `CaseManager` operations. Shared by the IPC handlers and the
 * application menu.
 */
export class CaseFlows {
  public constructor(private readonly caseManager: CaseManager) {}

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /**
   * The unsaved-changes guard: when the open case is dirty, ask the user to
   * save, discard, or cancel. Resolves to `true` when it is safe to proceed
   * (the case was clean, saved, or explicitly discarded).
   */
  public async confirmDiscard(window: BrowserWindow | null) {
    const state = this.caseManager.state();
    if (state === null || !state.dirty) return true;

    const options = {
      type: "warning",
      message: `Do you want to save the changes made to “${state.name}”?`,
      detail: "Your changes will be lost if you don't save them.",
      buttons: ["Save", "Don’t Save", "Cancel"],
      defaultId: 0,
      cancelId: 2,
    } satisfies MessageBoxOptions;
    const { response } =
      window === null
        ? await dialog.showMessageBox(options)
        : await dialog.showMessageBox(window, options);

    if (response === 2) return false;
    if (response === 0) await this.caseManager.save();
    return true;
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /**
   * Create a new case: guard the open one, pick a directory, create.
   * Resolves to the new case state, or `null` when cancelled.
   */
  public async newCase(window: BrowserWindow | null) {
    if (!(await this.confirmDiscard(window))) return null;
    const dir = await this.pickCaseDir(window, true);
    if (dir === undefined) return null;
    return this.caseManager.newCase(dir);
  }

  /**
   * Open a case: guard the open one, pick a directory, open.
   * Resolves to the opened case state, or `null` when cancelled.
   */
  public async openCase(window: BrowserWindow | null) {
    if (!(await this.confirmDiscard(window))) return null;
    const dir = await this.pickCaseDir(window, false);
    if (dir === undefined) return null;
    return this.caseManager.openCase(dir);
  }

  /**
   * Open the case in the given directory, guarding the open one.
   * Resolves to the opened case state, or `null` when cancelled.
   */
  public async openRecent(window: BrowserWindow | null, dir: string) {
    if (!(await this.confirmDiscard(window))) return null;
    return this.caseManager.openCase(dir);
  }

  /**
   * Close the case, guarding unsaved changes.
   */
  public async close(window: BrowserWindow | null) {
    if (!(await this.confirmDiscard(window))) return;
    this.caseManager.close();
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  // Ask the user for a case directory. `create` shows a save-style dialog
  // for a new directory; otherwise an existing directory is picked.
  private async pickCaseDir(window: BrowserWindow | null, create: boolean) {
    if (create) {
      const options = {
        title: "New Case",
        buttonLabel: "Create",
        nameFieldLabel: "Case Name",
        properties: ["createDirectory", "showOverwriteConfirmation"],
      } satisfies SaveDialogOptions;
      const result =
        window === null
          ? await dialog.showSaveDialog(options)
          : await dialog.showSaveDialog(window, options);
      return result.canceled ? undefined : result.filePath;
    }
    const options = {
      title: "Open Case",
      buttonLabel: "Open",
      properties: ["openDirectory"],
    } satisfies OpenDialogOptions;
    const result =
      window === null
        ? await dialog.showOpenDialog(options)
        : await dialog.showOpenDialog(window, options);
    return result.canceled ? undefined : result.filePaths[0];
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
