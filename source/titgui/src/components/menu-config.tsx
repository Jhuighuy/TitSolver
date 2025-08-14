/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { Badge, Box, Flex, Spinner, Text } from "@radix-ui/themes";
import { useMemo } from "react";
import { TbClipboardText, TbCopy, TbDownload } from "react-icons/tb";

import { ConfigTreeEditor } from "~/components/config-tree-editor";
import { type MenuAction, useMenuAction } from "~/components/menu";
import { useSolverConfigController } from "~/hooks/use-solver-config-controller";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export function ConfigMenu() {
  const {
    isLoading,
    isSaving,
    spec,
    tree,
    setTree,
    copyTree,
    pasteTree,
    exportTree,
  } = useSolverConfigController();

  // ---- Menu actions. --------------------------------------------------------

  const copyAction = useMemo<MenuAction>(
    () => ({
      name: "Copy JSON",
      icon: <TbCopy size={16} />,
      disabled: tree === null,
      onClick: () => {
        if (tree === null) return;
        void copyTree();
      },
    }),
    [copyTree, tree],
  );
  useMenuAction(copyAction);

  const pasteAction = useMemo<MenuAction>(
    () => ({
      name: "Paste JSON",
      icon: <TbClipboardText size={16} />,
      disabled: spec === null || tree === null,
      onClick: () => {
        if (spec === null) return;
        void pasteTree();
      },
    }),
    [pasteTree, spec, tree],
  );
  useMenuAction(pasteAction);

  const exportAction = useMemo<MenuAction>(
    () => ({
      name: "Export JSON",
      icon: <TbDownload size={16} />,
      disabled: tree === null,
      onClick: () => {
        if (tree === null) return;
        exportTree();
      },
    }),
    [exportTree, tree],
  );
  useMenuAction(exportAction);

  // ---- Loading. -------------------------------------------------------------

  if (isLoading || spec === null || tree === null) {
    return (
      <Flex direction="column" gap="3" p="3">
        <Flex align="center" gap="2">
          <Spinner size="2" />
          <Text size="2">Loading solver configuration…</Text>
        </Flex>
      </Flex>
    );
  }

  // ---- Layout. --------------------------------------------------------------

  return (
    <Flex direction="column" height="100%" gap="3">
      <Box px="2" pt="2">
        <Flex align="start" justify="between" gap="3">
          <Text as="div" size="2" weight="bold">
            Solver Setup
          </Text>
          <Badge color={isSaving ? "amber" : "green"} variant="soft">
            {isSaving ? "Saving" : "Synced"}
          </Badge>
        </Flex>
      </Box>

      <Box px="2" flexGrow="1" minHeight="0">
        <ConfigTreeEditor spec={spec} tree={tree} onTreeChange={setTree} />
      </Box>
    </Flex>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
