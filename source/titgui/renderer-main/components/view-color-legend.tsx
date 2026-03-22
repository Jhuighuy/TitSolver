/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { Box } from "@radix-ui/themes";
import type { ComponentProps } from "react";

import { TechText } from "~/renderer-common/components/basic";
import { ColorBox } from "~/renderer-common/components/color-box";
import { assert, iota } from "~/shared/utils";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

type ViewColorLegendProps = ComponentProps<typeof ColorBox> & {
  min: number;
  max: number;
  ticks?: number;
  title?: string;
};

export function ViewColorLegend({
  min,
  max,
  ticks = 10,
  title,
  ...props
}: Omit<ViewColorLegendProps, "orientation">) {
  assert(min <= max);
  assert(ticks >= 2);

  return (
    <ColorBox
      {...props}
      orientation="vertical"
      position="relative"
      style={{ border: "2px solid var(--gray-11)" }}
    >
      {/* ---- Title. ------------------------------------------------------ */}
      {title && (
        <Box
          position="absolute"
          left="-150%"
          top="50%"
          style={{ transform: "translate(-50%, -50%) rotate(-90deg)" }}
        >
          <TechText>{title}</TechText>
        </Box>
      )}

      {/* ---- Ticks. ------------------------------------------------------ */}
      {iota(ticks).map((index) => {
        const t = index / (ticks - 1);
        const offset = `calc(${1 - t} * (100% + 2px) - 1px)`;

        return (
          <Box
            key={index}
            position="absolute"
            left="100%"
            top={offset}
            width="100%"
            height="2px"
            style={{
              background: "var(--gray-11)",
              transform: "translate(-50%, -50%)",
            }}
          >
            <Box
              position="absolute"
              left="150%"
              style={{ transform: "translateY(-50%)" }}
            >
              <TechText size="2">{(min + (max - min) * t).toFixed(2)}</TechText>
            </Box>
          </Box>
        );
      })}
    </ColorBox>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
