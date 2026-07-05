/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { z } from "zod";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
 * Tab in the help window.
 */
export const helpTabSchema = z.object({
  /** Unique tab ID. */
  id: z.number(),

  /** URL of the tab. */
  url: z.string(),
});

export type HelpTab = z.infer<typeof helpTabSchema>;

/**
 * Session in the help window.
 */
export const helpSessionSchema = z.object({
  /** ID of the active tab. */
  activeTabID: z.number().optional(),

  /** Tabs in the session. */
  tabs: z.array(helpTabSchema),
});

export type HelpSession = z.infer<typeof helpSessionSchema>;

/**
 * Session listener function.
 */
export type HelpSessionListener = (session: HelpSession) => void;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
