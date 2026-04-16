/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { ScrollArea as BaseScrollArea } from "@base-ui/react/scroll-area";
import type { ComponentProps, Ref } from "react";

import { cn } from "~/renderer-common/components/utils";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

interface ScrollAreaProps extends ComponentProps<typeof BaseScrollArea.Root> {
  viewportRef?: Ref<HTMLDivElement>;
}

export function ScrollArea({
  className,
  children,
  viewportRef,
  ...props
}: Readonly<ScrollAreaProps>) {
  return (
    <BaseScrollArea.Root
      {...props}
      className={cn("relative h-full w-full overflow-hidden", className)}
    >
      <BaseScrollArea.Viewport ref={viewportRef} className="h-full w-full">
        {children}
      </BaseScrollArea.Viewport>
      <BaseScrollArea.Scrollbar
        orientation="vertical"
        className="flex w-2 touch-none p-0.5 select-none"
      >
        <BaseScrollArea.Thumb className="flex-1 rounded-full bg-(--bg-6) hover:bg-(--bg-5)" />
      </BaseScrollArea.Scrollbar>
      <BaseScrollArea.Scrollbar
        orientation="horizontal"
        className="flex h-2 touch-none flex-col p-0.5 select-none"
      >
        <BaseScrollArea.Thumb className="flex-1 rounded-full bg-(--bg-6) hover:bg-(--bg-5)" />
      </BaseScrollArea.Scrollbar>
    </BaseScrollArea.Root>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
