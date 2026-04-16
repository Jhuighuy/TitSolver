/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { IconAlertCircle } from "@tabler/icons-react";

import { Box, Flex } from "~/renderer-common/components/layout";
import { Separator } from "~/renderer-common/components/separator";
import { Text } from "~/renderer-common/components/text";
import { PropertyEditor } from "~/renderer-main/components/property-editor";
import { useProperties } from "~/renderer-main/hooks/use-properties";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export function SetupMenu() {
  const { spec, tree, isComplete, isLoading, setTree } = useProperties();

  return isLoading || spec === undefined || tree === undefined ? (
    <Box p="4">
      <Text color="muted">Loading…</Text>
    </Box>
  ) : (
    <Flex direction="column" size="100%">
      {!isComplete && (
        <>
          <Text size="1" color="red">
            <Flex align="center" gap="2" p="2">
              <IconAlertCircle />
              Configuration incomplete
            </Flex>
          </Text>

          <Separator size="full" />
        </>
      )}

      <Box flexGrow="1">
        <PropertyEditor
          spec={spec}
          tree={tree}
          onTreeChanged={(tree) => {
            setTree(tree);
          }}
        />
      </Box>
    </Flex>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
