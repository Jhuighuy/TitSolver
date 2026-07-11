/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import type { ReactNode } from "react";

import { Text } from "~/renderer/common/components/text";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

interface StatTileProps {
  label: string;
  icon?: ReactNode;
  value?: ReactNode;
}

/**
 * A labeled headline value: a label (with an optional icon) on the left, the
 * current value on the right.
 */
export function StatTile({ label, icon, value }: Readonly<StatTileProps>) {
  return (
    <div className="flex items-center justify-between px-1">
      <Text size="2" variant="label" color="muted">
        <span className="flex items-center gap-1">
          {icon}
          {label}
        </span>
      </Text>

      <Text size="2" weight="bold">
        {value}
      </Text>
    </div>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
