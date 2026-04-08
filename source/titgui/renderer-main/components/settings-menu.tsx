/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { Select } from "~/renderer-common/components/select";
import { TreeTable } from "~/renderer-common/components/tree-table";
import { useThemeState } from "~/renderer-common/hooks/use-theme";
import type { Theme } from "~/shared/theme";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export function SettingsMenu() {
  const [theme, setTheme] = useThemeState();

  return (
    <TreeTable.Root>
      <TreeTable.Node label="Appearance">
        <TreeTable.Node
          label="Theme"
          value={
            <Select.Root
              value={theme}
              onValueChange={(value: Theme) => {
                setTheme(value);
              }}
            >
              <Select.Trigger />
              <Select.Content>
                <Select.Item value="light">Light</Select.Item>
                <Select.Item value="dark">Dark</Select.Item>
                <Select.Item value="system">System</Select.Item>
              </Select.Content>
            </Select.Root>
          }
        />
      </TreeTable.Node>
    </TreeTable.Root>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
