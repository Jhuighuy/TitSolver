/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { Box } from "@radix-ui/themes";
import { type ComponentProps, useMemo } from "react";

import {
  type ColorMapName,
  colorMaps,
} from "~/renderer-common/visual/color-map";

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
