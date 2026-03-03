/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { useEffect, useState } from "react";

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
