/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import type { ComponentProps } from "react";

import { ColorBox } from "~/renderer/common/components/color-box";
import { Text } from "~/renderer/common/components/text";
import { cn } from "~/renderer/common/components/utils";
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
  className,
  ...props
}: Readonly<Omit<ViewColorLegendProps, "orientation">>) {
  assert(min <= max);
  assert(ticks >= 2);

  return (
    <ColorBox
      {...props}
      orientation="vertical"
      className={cn("relative border-2 text-(--neutral-8)", className)}
    >
      {/* ---- Title. ------------------------------------------------------ */}
      {title !== undefined && (
        <div
          className="absolute top-1/2"
          style={{
            left: "-150%",
            transform: "translate(-50%, -50%) rotate(-90deg)",
          }}
        >
          <Text size="3" mono>
            {title}
          </Text>
        </div>
      )}

      {/* ---- Ticks. ------------------------------------------------------ */}
      {iota(ticks).map((index) => {
        const t = index / (ticks - 1);
        const offset = `calc(${1 - t} * (100% + 2px) - 1px)`;

        return (
          <div
            key={index}
            className="absolute left-full h-0.5 w-full"
            style={{
              top: offset,
              background: "currentColor",
              transform: "translate(-50%, -50%)",
            }}
          >
            <div
              className="absolute"
              style={{ left: "150%", transform: "translateY(-50%)" }}
            >
              <Text size="2" mono>
                {(min + (max - min) * t).toFixed(2)}
              </Text>
            </div>
          </div>
        );
      })}
    </ColorBox>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
