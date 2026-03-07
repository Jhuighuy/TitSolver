/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { Box } from "@radix-ui/themes";
import { type ComponentProps, useMemo } from "react";

import { TechText } from "~/components/basic";
import { assert, iota } from "~/utils";
import { type ColorMapName, colorMaps } from "~/visual/color-map";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

type ColorBoxProps = ComponentProps<typeof Box> & {
  name: ColorMapName;
  orientation?: "horizontal" | "vertical";
};

export function ColorBox({
  name,
  orientation = "horizontal",
  style,
  ...props
}: ColorBoxProps) {
  const gradientStops = useMemo(
    () =>
      colorMaps[name].points
        .map(([value, r, g, b]) => {
          r = Math.round(r * 255);
          g = Math.round(g * 255);
          b = Math.round(b * 255);
          return `rgb(${r}, ${g}, ${b}) ${value * 100}%`;
        })
        .join(", "),
    [name],
  );

  const gradient = useMemo(
    () =>
      orientation === "vertical"
        ? `linear-gradient(to top, ${gradientStops})`
        : `linear-gradient(to right, ${gradientStops})`,
    [gradientStops, orientation],
  );

  return <Box {...props} style={{ ...style, background: gradient }} />;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

type ColorLegendProps = ColorBoxProps & {
  min: number;
  max: number;
  ticks?: number;
  title?: string;
};

export function ColorLegend({
  min,
  max,
  ticks = 10,
  title,
  ...props
}: Omit<ColorLegendProps, "orientation">) {
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
