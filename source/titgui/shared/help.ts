/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
 * Tab in the help window.
 */
export interface HelpTab {
  /** Unique tab ID. */
  id: number;

  /** URL of the tab. */
  url: string;
}

/**
 * Session in the help window.
 */
export interface HelpSession {
  /** ID of the active tab. */
  activeTabID?: number;

  /** Tabs in the session. */
  tabs: HelpTab[];
}

/**
 * Session listener function.
 */
export type HelpSessionListener = (session: HelpSession) => void;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
 * Help service.
 */
export interface HelpService {
  /** Get the current session. */
  getSession(): Promise<HelpSession>;

  /** Add a new tab. */
  addTab(url?: string): Promise<void>;

  /** Close the tab with the given ID. */
  closeTab(id: number): Promise<void>;

  /** Select the tab with the given ID. */
  selectTab(id: number): Promise<void>;

  /** Update the URL of the tab with the given ID. */
  navigateTab(id: number, url?: string): Promise<void>;

  /** Subscribe to session changes. */
  onSessionChanged(listener: HelpSessionListener): () => void;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
