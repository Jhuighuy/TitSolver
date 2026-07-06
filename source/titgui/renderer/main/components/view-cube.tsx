/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { type ComponentProps, Fragment } from "react";
import { Euler, Quaternion, Vector3 } from "three";

import { Strong, Text } from "~/renderer/common/components/text";
import { cn } from "~/renderer/common/components/utils";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

type ViewCubeProps = ComponentProps<"div"> & {
  length?: string;
  diameter?: string;
  rotation: Euler;
  setRotation?: (rotation: Euler) => void;
};

export function ViewCube({
  length = "30%",
  diameter = "20%",
  rotation,
  setRotation,
  className,
  ...props
}: Readonly<ViewCubeProps>) {
  const quat = new Quaternion().setFromEuler(rotation).invert();
  return (
    <div {...props} className={cn("relative", className)}>
      {Object.entries(axisCns).map(([axis, [colorCn, altColorCn]], i) => {
        const e = new Vector3().setComponent(i, 1).applyQuaternion(quat);
        e.y *= -1;
        const scale = Math.hypot(e.x, e.y);
        const angle = Math.atan2(e.y, e.x);

        return ([1, -1] as const).map((sign) => {
          const signedAxis = `${sign > 0 ? "+" : "-"}${axis}`;

          return (
            <Fragment key={signedAxis}>
              <div
                onClick={() => setRotation?.(axisRotations[signedAxis])}
                className={cn(
                  "absolute flex items-center justify-center rounded-full",
                  sign > 0 ? colorCn : altColorCn,
                )}
                style={{
                  left: `calc(50% + ${sign * e.x} * ${length})`,
                  top: `calc(50% + ${sign * e.y} * ${length})`,
                  width: diameter,
                  height: diameter,
                  zIndex: 10 + Math.round(sign * e.z * 2),
                  transform: "translate(-50%, -50%)",
                }}
              >
                {sign > 0 && (
                  <Text className="text-(--neutral-8)">
                    <Strong>{axis.toUpperCase()}</Strong>
                  </Text>
                )}
              </div>

              {scale > Number.EPSILON && (
                <div
                  className={cn("absolute top-1/2 left-1/2 h-0.5", colorCn)}
                  style={{
                    width: `calc(${length} * ${scale} - ${diameter} / 2)`,
                    zIndex: 10 + Math.round(sign * e.z),
                    transform: [
                      "translate(-50%, -50%)",
                      `rotate(${angle}rad)`,
                      `translate(${sign * 50}%, 0)`,
                    ].join(" "),
                  }}
                />
              )}
            </Fragment>
          );
        });
      })}
    </div>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

const axisRotations: Record<string, Euler> = {
  "+x": new Euler(0, Math.PI / 2, 0),
  "-x": new Euler(0, -Math.PI / 2, 0),
  "+y": new Euler(Math.PI / 2, 0, 0),
  "-y": new Euler(-Math.PI / 2, 0, 0),
  "+z": new Euler(0, 0, 0),
  "-z": new Euler(0, Math.PI, 0),
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Axis colors deliberately stay raw (red/green/blue = X/Y/Z is a CAE
// convention) and are not part of the theme ramps.
const axisCns: Record<"x" | "y" | "z", [string, string]> = {
  x: [
    "bg-red-500/75 hover:bg-red-500",
    "border-2 border-red-500/75 hover:border-red-500",
  ],
  y: [
    "bg-green-500/75 hover:bg-green-500",
    "border-2 border-green-500/75 hover:border-green-500",
  ],
  z: [
    "bg-blue-500/75 hover:bg-blue-500",
    "border-2 border-blue-500/75 hover:border-blue-500",
  ],
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
