/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { Switch as BaseSwitch } from "@base-ui/react/switch";
import type { ComponentProps } from "react";

import { cn } from "~/renderer-common/components/utils";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export function Switch({
  className,
  ...props
}: Readonly<ComponentProps<typeof BaseSwitch.Root>>) {
  return (
    <BaseSwitch.Root
      {...props}
      className={cn(
        "relative inline-flex h-4 w-7 cursor-pointer items-center rounded-full border border-(--chrome-1) bg-(--bg-6) transition-colors focus-visible:outline-2 focus-visible:outline-offset-1 focus-visible:outline-(--accent-fg-3) disabled:cursor-not-allowed disabled:opacity-40 data-checked:border-(--accent-fg-3) data-checked:bg-(--accent-bg-3)",
        className,
      )}
    >
      <BaseSwitch.Thumb className="block h-3 w-3 translate-x-0.5 rounded-full bg-(--bg-1) shadow-sm transition-transform data-checked:translate-x-3" />
    </BaseSwitch.Root>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
