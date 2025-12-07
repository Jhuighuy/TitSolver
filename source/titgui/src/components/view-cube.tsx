/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { Box, Flex, Strong, Text } from "@radix-ui/themes";
import { type ComponentProps, Fragment } from "react";
import { Euler, Quaternion, Vector3 } from "three";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

type ViewCubeProps = ComponentProps<typeof Box> & {
  length?: string;
  diameter?: string;
  rotation: Vector3;
  setRotation?: (rotation: Vector3) => void;
};

export function ViewCube({
  length = "30%",
  diameter = "20%",
  rotation,
  setRotation,
  ...props
}: Readonly<ViewCubeProps>) {
  const rotationRad = rotation.clone().multiplyScalar(Math.PI / 180);
  const euler = new Euler().setFromVector3(rotationRad);
  const quat = new Quaternion().setFromEuler(euler).invert();

  return (
    <Box {...props} position="relative">
      {Object.entries(axisCns).map(([axis, [colorCn, altColorCn]], i) => {
        const e = new Vector3().setComponent(i, 1).applyQuaternion(quat);
        e.y *= -1;
        const scale = Math.hypot(e.x, e.y);
        const angle = Math.atan2(e.y, e.x);

        return [+1, -1].map((sign) => {
          const signedAxis = `${sign > 0 ? "+" : "-"}${axis}`;

          return (
            <Fragment key={signedAxis}>
              <Flex
                align="center"
                justify="center"
                position="absolute"
                left={`calc(50% + ${sign * e.x} * ${length})`}
                top={`calc(50% + ${sign * e.y} * ${length})`}
                width={diameter}
                height={diameter}
                onClick={() => setRotation?.(axisRotations[signedAxis])}
                className={sign > 0 ? colorCn : altColorCn}
                style={{
                  borderRadius: "100%",
                  zIndex: 10 + Math.round(sign * e.z * 2),
                  transform: "translate(-50%, -50%)",
                }}
              >
                {sign > 0 && (
                  <Text size="1" className="text-gray-800">
                    <Strong>{axis.toUpperCase()}</Strong>
                  </Text>
                )}
              </Flex>

              {scale > Number.EPSILON && (
                <Box
                  position="absolute"
                  left="50%"
                  top="50%"
                  width={`calc(${length} * ${scale} - ${diameter} / 2)`}
                  height="2px"
                  className={colorCn}
                  style={{
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
    </Box>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

const axisRotations: Record<string, Vector3> = {
  "+x": new Vector3(0, 90, 0),
  "-x": new Vector3(0, -90, 0),
  "+y": new Vector3(90, 0, 0),
  "-y": new Vector3(-90, 0, 0),
  "+z": new Vector3(0, 0, 0),
  "-z": new Vector3(0, 180, 0),
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

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
