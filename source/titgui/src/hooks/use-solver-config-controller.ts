/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { useCallback, useEffect, useRef } from "react";

import { useSolverConfig } from "~/hooks/use-solver-config";
import { logger } from "~/logging";
import {
  type PropertyRecord,
  isPropertyTreeCompatible,
  normalizeRootTree,
} from "~/solver-config";
import {
  copyJsonToClipboard,
  exportJson,
  readJsonFromClipboard,
} from "~/solver-config-io";
import type { SetStateAction } from "~/utils";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export function useSolverConfigController() {
  const [isLoading, isSaving, spec, tree, setTree] = useSolverConfig();
  const hasNormalizedOnLoad = useRef(false);

  // Normalize incoming config once, right after the first successful load. The
  // editor can then assume a stable tree shape during normal interaction.
  useEffect(() => {
    if (
      isLoading ||
      spec === null ||
      tree === null ||
      hasNormalizedOnLoad.current
    ) {
      return;
    }

    hasNormalizedOnLoad.current = true;
    const normalized = normalizeRootTree(spec, tree);
    if (normalized.issues.length === 0) return;

    for (const issue of normalized.issues) {
      logger.warn(
        `Reset config ${issue.path}: expected ${issue.expected}, got ${issue.actual}.`,
      );
    }

    setTree(normalized.tree);
  }, [isLoading, setTree, spec, tree]);

  const copyTree = useCallback(async () => {
    if (tree === null) return;
    await copyJsonToClipboard(tree);
  }, [tree]);

  const pasteTree = useCallback(async () => {
    if (spec === null) return;
    const next = await readJsonFromClipboard();
    if (!isPropertyTreeCompatible(spec, next)) {
      logger.warn("Clipboard JSON is not compatible with the solver config.");
      return;
    }
    setTree(next as PropertyRecord);
  }, [setTree, spec]);

  const exportTree = useCallback(() => {
    if (tree === null) return;
    exportJson("solver-config.json", tree);
  }, [tree]);

  return {
    isLoading,
    isSaving,
    spec,
    tree,
    setTree: setTree as (next: SetStateAction<PropertyRecord>) => void,
    copyTree,
    pasteTree,
    exportTree,
  };
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
