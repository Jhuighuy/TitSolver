/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { Box, Theme, type ThemeProps } from "@radix-ui/themes";
import type { Euler } from "three";

import type {
  ColorMapName,
  ColorRange,
} from "~/renderer-common/visual/color-map";
import { ViewColorLegend } from "~/renderer-main/components/view-color-legend";
import { ViewCube } from "~/renderer-main/components/view-cube";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

type ViewHUDProps = {
  appearance: ThemeProps["appearance"];
  cameraRotation: Euler;
  setCameraRotation: (value: Euler) => void;
  colorMapName: ColorMapName;
  colorRange: ColorRange;
  colorTitle: string;
};

export function ViewHUD({
  appearance,
  cameraRotation,
  setCameraRotation,
  colorMapName,
  colorRange,
  colorTitle,
}: Readonly<ViewHUDProps>) {
  return (
    <Theme appearance={appearance}>
      <Box
        position="absolute"
        right="8"
        top="8"
        style={{ transform: "translate(50%, -50%)" }}
      >
        <ViewCube
          width="100px"
          height="100px"
          rotation={cameraRotation}
          setRotation={setCameraRotation}
        />
      </Box>

      <Box
        position="absolute"
        left="8"
        top="50%"
        style={{ transform: "translate(-50%, -50%)" }}
      >
        <ViewColorLegend
          name={colorMapName}
          title={colorTitle}
          min={colorRange.min}
          max={colorRange.max}
          width="20px"
          height="500px"
        />
      </Box>
    </Theme>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
