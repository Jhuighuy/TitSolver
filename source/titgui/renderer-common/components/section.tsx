/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { IconChevronRight } from "@tabler/icons-react";
import { type ReactNode, useState } from "react";

import { Flex } from "~/renderer-common/components/layout";
import { Text } from "~/renderer-common/components/text";
import { cn } from "~/renderer-common/components/utils";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

const sizeConfig = {
  "1": {
    chevronClassName: "size-3",
    labelSize: "1",
    gap: "1",
    childGap: "1",
    indent: "3",
    borderOffset: "1",
  },
  "2": {
    chevronClassName: "size-4",
    labelSize: "3",
    gap: "2",
    childGap: "2",
    indent: "3",
    borderOffset: "2",
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
  const { chevronClassName, labelSize, gap, childGap, indent, borderOffset } =
    sizeConfig[size];

  return (
    <Flex direction="column" gap={gap}>
      {/* ---- Trigger. ------------------------------------------------------ */}
      <button
        type="button"
        onClick={() => {
          setOpen((prev) => !prev);
        }}
        className="group flex w-full cursor-pointer items-center gap-1.5 text-left text-(--fg-5) transition-colors select-none hover:text-(--fg-3)"
      >
        <IconChevronRight
          className={cn(
            "shrink-0 transition-transform duration-200",
            chevronClassName,
            open ? "rotate-90" : "rotate-0",
          )}
        />
        <Text size={labelSize} className="group-hover:text-(--fg-2)">
          {label}
        </Text>
      </button>

      {/* ---- Content. ---------------------------------------------------- */}
      <div
        className="grid transition-[grid-template-rows] duration-200 ease-in-out"
        style={{ gridTemplateRows: open ? "1fr" : "0fr" }}
      >
        <Flex
          direction="column"
          gap={childGap}
          pl={indent}
          ml={borderOffset}
          className="flex flex-col overflow-hidden border-l border-(--chrome-1)"
        >
          {children}
        </Flex>
      </div>
    </Flex>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
