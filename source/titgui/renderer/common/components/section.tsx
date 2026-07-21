/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { IconChevronRight } from "@tabler/icons-react";
import { type ReactNode, useState } from "react";

import { Text } from "~/renderer/common/components/text";
import { cn } from "~/renderer/common/components/utils";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

const sizeConfig = {
  "1": {
    chevronClassName: "size-3",
    labelSize: "1",
    rootClassName: "gap-1",
    childClassName: "gap-1 pl-3 ml-1",
  },
  "2": {
    chevronClassName: "size-4",
    labelSize: "3",
    rootClassName: "gap-2",
    childClassName: "gap-2 pl-3 ml-2",
  },
} as const;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

interface SectionProps {
  label: string;
  size?: "1" | "2";
  children: ReactNode;
  defaultOpen?: boolean;
}

export function Section({
  label,
  size = "1",
  children,
  defaultOpen = true,
}: Readonly<SectionProps>) {
  const [open, setOpen] = useState(defaultOpen);
  const { chevronClassName, labelSize, rootClassName, childClassName } =
    sizeConfig[size];

  return (
    <div className={cn("flex flex-col", rootClassName)}>
      {/* ---- Trigger. ------------------------------------------------------ */}
      <button
        type="button"
        onClick={() => {
          setOpen((prev) => !prev);
        }}
        className="group flex w-full cursor-pointer items-center gap-1.5 text-left text-(--neutral-5) transition-colors select-none hover:text-(--neutral-7)"
      >
        <IconChevronRight
          className={cn(
            "shrink-0 transition-transform duration-200",
            chevronClassName,
            open ? "rotate-90" : "rotate-0",
          )}
        />
        <Text size={labelSize} className="group-hover:text-(--neutral-8)">
          {label}
        </Text>
      </button>

      {/* ---- Content. ---------------------------------------------------- */}
      <div
        className="grid transition-[grid-template-rows] duration-200 ease-in-out"
        style={{ gridTemplateRows: open ? "1fr" : "0fr" }}
      >
        <div
          className={cn(
            "flex flex-col overflow-hidden border-l border-(--neutral-4)",
            childClassName,
          )}
        >
          {children}
        </div>
      </div>
    </div>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
