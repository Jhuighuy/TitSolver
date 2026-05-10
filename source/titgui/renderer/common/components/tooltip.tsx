/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { Tooltip as BaseTooltip } from "@base-ui/react/tooltip";
import type { ComponentProps, ReactNode } from "react";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

interface TooltipProps {
  content: ReactNode;
  side?: ComponentProps<typeof BaseTooltip.Positioner>["side"];
  children: ReactNode;
}

export function Tooltip({
  content,
  side = "top",
  children,
}: Readonly<TooltipProps>) {
  return (
    <BaseTooltip.Provider delay={400} closeDelay={100}>
      <BaseTooltip.Root>
        <BaseTooltip.Trigger render={<span className="inline-flex" />}>
          {children}
        </BaseTooltip.Trigger>
        <BaseTooltip.Portal>
          <BaseTooltip.Positioner side={side} sideOffset={6} align="center">
            <BaseTooltip.Popup className="z-50 max-w-xs rounded border border-(--chrome-1) bg-(--bg-3) px-2 py-1 text-(length:--text-1) leading-(--leading-1) text-(--fg-1) shadow-md">
              {content}
            </BaseTooltip.Popup>
          </BaseTooltip.Positioner>
        </BaseTooltip.Portal>
      </BaseTooltip.Root>
    </BaseTooltip.Provider>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
