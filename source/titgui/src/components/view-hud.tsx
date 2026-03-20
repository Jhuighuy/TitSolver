/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { Box, Theme, type ThemeProps } from "@radix-ui/themes";
import type { Euler } from "three";

import type { MouseMode } from "~/components/mouse-mode";
import type { SelectionModifier } from "~/components/selection";
import { ColorLegend } from "~/components/color-bar";
import { ViewCube } from "~/components/view-cube";
import type { ColorMapName, ColorRange } from "~/visual/color-map";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

type ViewHUDProps = {
  appearance: ThemeProps["appearance"];
  cameraRotation: Euler;
  setCameraRotation: (value: Euler) => void;
  colorMapName: ColorMapName;
  colorRange: ColorRange;
  colorTitle: string;
  mouseMode: MouseMode;
  selectionModifier: SelectionModifier;
};

export function ViewHUD({
  appearance,
  cameraRotation,
  setCameraRotation,
  colorMapName,
  colorRange,
  colorTitle,
  mouseMode,
  selectionModifier,
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
        <ColorLegend
          name={colorMapName}
          title={colorTitle}
          min={colorRange.min}
          max={colorRange.max}
          width="20px"
          height="500px"
        />
      </Box>

      <SelectionHint
        mouseMode={mouseMode}
        selectionModifier={selectionModifier}
      />
    </Theme>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

function SelectionHint({
  mouseMode,
  selectionModifier,
}: Readonly<{
  mouseMode: MouseMode;
  selectionModifier: SelectionModifier;
}>) {
  if (mouseMode === "normal") return null;

  return (
    <Box
      position="absolute"
      right="3"
      bottom="3"
      px="3"
      py="2"
      style={{
        pointerEvents: "none",
        borderRadius: "var(--radius-3)",
        background:
          "color-mix(in srgb, var(--color-panel-solid) 92%, var(--accent-3))",
        boxShadow: "0 6px 20px rgb(0 0 0 / 0.18)",
        color: "var(--gray-12)",
        fontSize: "12px",
        lineHeight: 1.35,
      }}
    >
      <div>
        {mouseMode === "rect" ? "Rectangle selection" : "Lasso selection"}:{" "}
        <strong>{selectionModifierLabel(selectionModifier)}</strong>
      </div>
      <div>Drag to select. Shift adds, Alt subtracts.</div>
    </Box>
  );
}

function selectionModifierLabel(modifier: SelectionModifier) {
  switch (modifier) {
    case "replace":
      return "Replace";
    case "add":
      return "Add";
    case "subtract":
      return "Subtract";
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
