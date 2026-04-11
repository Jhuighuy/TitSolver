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

const SESSION = "session";
export const SESSION_FRAME_COUNT_CHANNEL = `${SESSION}:frame:count`;
export const SESSION_FRAME_GET_CHANNEL = `${SESSION}:frame:get`;
export const SESSION_EXPORT_RUN_CHANNEL = `${SESSION}:export:run`;
export const SESSION_SOLVER_RUN_CHANNEL = `${SESSION}:solver:run`;
export const SESSION_SOLVER_STOP_CHANNEL = `${SESSION}:solver:stop`;
export const SESSION_SOLVER_IS_RUNNING_CHANNEL = `${SESSION}:solver:is-running`;
export const SESSION_SOLVER_EVENT_CHANNEL = `${SESSION}:solver:event`;

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
