/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { Menu, type MenuItemConstructorOptions } from "electron";

import type { CaseState, RecentCase } from "~/shared/case";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
 * Actions the application menu triggers; wired by the application.
 */
export interface AppMenuHandlers {
  newCase: () => void;
  openCase: () => void;
  openRecentCase: (dir: string) => void;
  saveCase: () => void;
  closeCase: () => void;
  openHelp: () => void;
}

/**
 * Case-dependent state the menu reflects.
 */
export interface AppMenuState {
  caseState: CaseState;
  recents: RecentCase[];
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
 * Install (or refresh) the application menu. Called at startup and on every
 * case state change, so the recents submenu and item enablement stay
 * current. This is what makes the standard shortcuts (⌘N/⌘O/⌘S, copy and
 * paste, zoom, full screen) work.
 */
export function updateApplicationMenu(
  handlers: AppMenuHandlers,
  state: AppMenuState,
) {
  const caseOpen = state.caseState !== null;

  const template: MenuItemConstructorOptions[] = [
    // ---- App (macOS only). ---------------------------------------------------
    ...(process.platform === "darwin"
      ? [{ role: "appMenu" } satisfies MenuItemConstructorOptions]
      : []),

    // ---- File. -----------------------------------------------------------------
    {
      label: "File",
      submenu: [
        {
          label: "New Case…",
          accelerator: "CmdOrCtrl+N",
          click: handlers.newCase,
        },
        {
          label: "Open Case…",
          accelerator: "CmdOrCtrl+O",
          click: handlers.openCase,
        },
        {
          label: "Open Recent",
          enabled: state.recents.length > 0,
          submenu: state.recents.map((recent) => ({
            label: recent.name,
            sublabel: recent.dir,
            click: () => {
              handlers.openRecentCase(recent.dir);
            },
          })),
        },
        { type: "separator" },
        {
          label: "Save Case",
          accelerator: "CmdOrCtrl+S",
          enabled: caseOpen,
          click: handlers.saveCase,
        },
        {
          label: "Close Case",
          accelerator: "CmdOrCtrl+Shift+W",
          enabled: caseOpen,
          click: handlers.closeCase,
        },
        { type: "separator" },
        process.platform === "darwin" ? { role: "close" } : { role: "quit" },
      ],
    },

    // ---- Standard menus. -------------------------------------------------------
    { role: "editMenu" },
    { role: "viewMenu" },
    { role: "windowMenu" },

    // ---- Help. -----------------------------------------------------------------
    // A plain label, not `role: "help"` — macOS silently drops the role-based
    // Help menu for unsigned builds.
    {
      label: "Help",
      submenu: [
        {
          label: "BlueTit Manual",
          click: handlers.openHelp,
        },
      ],
    },
  ];

  Menu.setApplicationMenu(Menu.buildFromTemplate(template));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
