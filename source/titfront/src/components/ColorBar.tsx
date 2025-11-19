/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { Box } from "@radix-ui/themes";
import type { ComponentProps, CSSProperties } from "react";

import { TechnicalText } from "~/components/Basic";
import { iota, type Orientation } from "~/utils";
import { type ColorMapName, colorMaps } from "~/visual/ColorMap";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

type ColorBoxProps = ComponentProps<typeof Box> & {
  name: ColorMapName;
  orientation?: Orientation;
  style?: CSSProperties;
};

export function ColorBox({
  name,
  orientation = "horizontal",
  ...props
}: ColorBoxProps) {
  const gradientStops = colorMaps[name].points
    .map(
      ([v, r, g, b]) =>
        `rgb(
          ${Math.round(r * 255)},
          ${Math.round(g * 255)},
          ${Math.round(b * 255)}
        ) ${v * 100}%`
    )
    .join(", ");

  const gradient =
    orientation === "vertical"
      ? `linear-gradient(to top, ${gradientStops})`
      : `linear-gradient(to right, ${gradientStops})`;

  return <Box {...props} style={{ ...props.style, background: gradient }} />;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

type ColorLegendProps = ColorBoxProps & {
  min: number;
  max: number;
  ticks: number;
  title?: string;
};

export function ColorLegendProps({
  min,
  max,
  ticks,
  title,
  orientation = "horizontal",
  ...props
}: ColorLegendProps) {
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
          <TechnicalText color="gray">{title}</TechnicalText>
        </Box>
      )}

      {/* ---- Ticks. ------------------------------------------------------ */}
      {iota(ticks).map((index) => {
        const t = index / (ticks - 1);
        const value = min + (max - min) * t;
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
              <TechnicalText size="2" color="gray">
                {value.toFixed(2)}
              </TechnicalText>
            </Box>
          </Box>
        );
      })}
    </ColorBox>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
