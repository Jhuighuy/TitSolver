/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

const THEME = "theme";
export const THEME_GET_CHANNEL = `${THEME}:get`;
export const THEME_SET_CHANNEL = `${THEME}:set`;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

const WINDOW = "window";
export const WINDOW_PERSIST_GET_CHANNEL = `${WINDOW}:persist-get`;
export const WINDOW_PERSIST_SET_CHANNEL = `${WINDOW}:persist-set`;
export const WINDOW_IS_FULL_SCREEN_CHANNEL = `${WINDOW}:is-full-screen`;
export const WINDOW_FULL_SCREEN_CHANGED_CHANNEL = `${WINDOW}:full-screen-changed`;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

const HELP = "help";
export const HELP_GET_SESSION_CHANNEL = `${HELP}:get-session`;
export const HELP_SESSION_CHANGED_CHANNEL = `${HELP}:session-changed`;
export const HELP_ADD_TAB_CHANNEL = `${HELP}:add-tab`;
export const HELP_CLOSE_TAB_CHANNEL = `${HELP}:close-tab`;
export const HELP_SELECT_TAB_CHANNEL = `${HELP}:select-tab`;
export const HELP_NAVIGATE_TAB_CHANNEL = `${HELP}:navigate-tab`;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

const WEBVIEW = "webview";
export const WEBVIEW_OPEN_IN_TAB_CHANNEL = `${WEBVIEW}:open-in-tab`;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
