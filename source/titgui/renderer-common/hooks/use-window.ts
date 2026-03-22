/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { useEffect, useState } from "react";
import { z } from "zod";

import { usePersistedState } from "~/renderer-common/hooks/use-persisted-state";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export function useWindowIsFullScreen() {
  const [isFullScreen, setIsFullScreen] = useState(true);

  useEffect(() => {
    if (window.windowState === undefined) return;

    let mounted = true;
    void window.windowState.isFullScreen().then((isFullScreen) => {
      if (mounted) setIsFullScreen(isFullScreen);
    });

    const unsubscribe = window.windowState.onFullScreenChanged((isFullScreen) =>
      setIsFullScreen(isFullScreen),
    );
    return () => {
      mounted = false;
      unsubscribe();
    };
  }, []);

  return isFullScreen;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

const appearanceSchema = z.union([
  z.literal("light"),
  z.literal("dark"),
  z.literal("system"),
]);

export type Appearance = z.infer<typeof appearanceSchema>;

export function useWindowAppearanceState() {
  return usePersistedState("appearance", appearanceSchema, "system");
}

export function useWindowAppearance() {
  const [appearance] = useWindowAppearanceState();
  const prefersDarkAppearance = useWindowPrefersDarkAppearance();

  if (appearance === "system") return prefersDarkAppearance ? "dark" : "light";
  return appearance;
}

function useWindowPrefersDarkAppearance() {
  const [prefersDarkAppearance, setPrefersDarkAppearance] = useState(
    window.matchMedia("(prefers-color-scheme: dark)").matches,
  );

  useEffect(() => {
    const listener = (event: MediaQueryListEvent) => {
      setPrefersDarkAppearance(event.matches);
    };

    const mediaQuery = window.matchMedia("(prefers-color-scheme: dark)");
    mediaQuery.addEventListener("change", listener);
    return () => mediaQuery.removeEventListener("change", listener);
  }, []);

  return prefersDarkAppearance;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
