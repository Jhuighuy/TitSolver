/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

const WINDOW = "window";
const PERSIST = "persist";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export const IPC_IS_FULL_SCREEN = `${WINDOW}:is-full-screen`;
export const IPC_FULL_SCREEN_CHANGED = `${WINDOW}:full-screen-changed`;
export const IPC_PERSIST_GET = `${PERSIST}:get`;
export const IPC_PERSIST_SET = `${PERSIST}:set`;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export const WINDOW_X_KEY = `${WINDOW}.x`;
export const WINDOW_Y_KEY = `${WINDOW}.y`;
export const WINDOW_WIDTH_KEY = `${WINDOW}.width`;
export const WINDOW_HEIGHT_KEY = `${WINDOW}.height`;
export const WINDOW_MAXIMIZED_KEY = `${WINDOW}.is-maximized`;
export const WINDOW_FULLSCREEN_KEY = `${WINDOW}.is-full-screen`;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
