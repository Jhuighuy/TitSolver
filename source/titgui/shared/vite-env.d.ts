/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/// <reference types="vite/client" />
/// <reference types="vite-plugin-svgr/client" />
import type { HelpService } from "~/shared/help";
import type { SolverEvent } from "~/shared/solver";
import type { Frame } from "~/shared/storage";
import type { Theme } from "~/shared/theme";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

interface ThemeAPI {
  get: () => Promise<Theme>;
  set: (theme: Theme) => Promise<void>;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

interface WindowStateAPI {
  persistGet: (key: string) => Promise<unknown>;
  persistSet: (key: string, value: unknown) => Promise<void>;
  isFullScreen: () => Promise<boolean>;
  onFullScreenChanged: (listener: (fullScreen: boolean) => void) => () => void;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

interface SessionAPI {
  getFrameCount: () => Promise<number>;
  getFrame: (index: number) => Promise<Frame>;
  export: () => Promise<void>;
  runSolver: () => Promise<void>;
  stopSolver: () => Promise<void>;
  isSolverRunning: () => Promise<boolean>;
  onSolverEvent: (listener: (event: SolverEvent) => void) => () => void;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

declare global {
  var theme: ThemeAPI | undefined;
  var windowState: WindowStateAPI | undefined;
  var session: SessionAPI | undefined;
  var help: HelpService | undefined;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
