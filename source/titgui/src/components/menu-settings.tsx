/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { Box, DataList, Select } from "@radix-ui/themes";

import { type Appearance, useWindowAppearanceState } from "~/hooks/use-window";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export function SettingsMenu() {
  const [appearance, setAppearance] = useWindowAppearanceState();
  return (
    <Box m="1">
      <DataList.Root size="1">
        <DataList.Item align="center">
          <DataList.Label>Appearance</DataList.Label>
          <DataList.Value>
            <Select.Root
              size="1"
              value={appearance}
              onValueChange={(value) => setAppearance(value as Appearance)}
            >
              <Select.Trigger />
              <Select.Content>
                <Select.Item value="light">Light</Select.Item>
                <Select.Item value="dark">Dark</Select.Item>
                <Select.Item value="system">System</Select.Item>
              </Select.Content>
            </Select.Root>
          </DataList.Value>
        </DataList.Item>
      </DataList.Root>
    </Box>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
