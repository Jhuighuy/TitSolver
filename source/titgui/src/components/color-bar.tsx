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
        .map(
          ([value, r, g, b]) =>
            `rgb(
              ${Math.round(r * 255)},
              ${Math.round(g * 255)},
              ${Math.round(b * 255)}
            ) ${value * 100}%`
        )
        .join(", "),
    [name]
  );

  const gradient = useMemo(
    () =>
      orientation === "vertical"
        ? `linear-gradient(to top, ${gradientStops})`
        : `linear-gradient(to right, ${gradientStops})`,
    [gradientStops, orientation]
  );

  return <Box {...props} style={{ ...style, background: gradient }} />;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

type ColorLegendProps = ColorBoxProps & {
  min: number;
  max: number;
  ticks: number;
  title?: string;
};

export function ColorLegend({
  min,
  max,
  ticks,
  title,
  orientation = "horizontal",
  ...props
}: ColorLegendProps) {
  assert(min <= max);
  assert(ticks >= 2);

  const reversed = orientation === "vertical";

  return (
    <ColorBox {...props} orientation={orientation} position="relative">
      {/* ---- Title. ------------------------------------------------------ */}
      {title && (
        <Box
          position="absolute"
          {...(orientation === "vertical"
            ? {
                left: "-100%",
                top: "50%",
                style: { transform: "translate(-50%, -50%) rotate(-90deg)" },
              }
            : {
                left: "50%",
                top: "-100%",
                style: { transform: "translate(-50%, -50%)" },
              })}
        >
          <TechText color="gray">{title}</TechText>
        </Box>
      )}

      {/* ---- Ticks. ------------------------------------------------------ */}
      {iota(ticks).map((index) => {
        const t = index / (ticks - 1);
        const offset = `calc(${reversed ? 1 - t : t} * (100% - 2px) + 1px)`;

        return (
          <Box
            key={index}
            position="absolute"
            {...(orientation === "vertical"
              ? { left: "100%", top: offset, width: "100%", height: "2px" }
              : { left: offset, top: "100%", width: "2px", height: "100%" })}
            style={{
              background: "var(--gray-11)",
              transform: "translate(-50%, -50%)",
            }}
          >
            <Box
              position="absolute"
              {...(orientation === "vertical"
                ? { left: "275%" }
                : { left: "50%", top: "150%" })}
              style={{ transform: "translate(-50%, -50%)" }}
            >
              <TechText size="2" color="gray">
                {(min + (max - min) * t).toFixed(2)}
              </TechText>
            </Box>
          </Box>
        );
      })}
    </ColorBox>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
