/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { Fragment } from "react";
import { Euler, Quaternion, Vector3 } from "three";

import { cn } from "~/utils";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export interface ViewCubeProps {
  rotation: Vector3;
  setRotation: (rotation: Vector3) => void;
  zIndex?: number;
  diamPx?: number;
  distPx?: number;
}

export function ViewCube({
  rotation,
  setRotation,
  zIndex: baseZ = 20,
  diamPx = 20,
  distPx = 30,
}: ViewCubeProps) {
  const rotationRad = rotation.clone().multiplyScalar(Math.PI / 180);
  const euler = new Euler().setFromVector3(rotationRad);
  const q = new Quaternion().setFromEuler(euler).invert();

  const axes: Record<"x" | "y" | "z", [string, string]> = {
    x: [
      "bg-red-500/75 hover:bg-red-500",
      "bg-gray-700/75 border-2 border-red-500/75 hover:border-red-500",
    ],
    y: [
      "bg-green-500/75 hover:bg-green-500",
      "bg-gray-700/75 border-2 border-green-500/75 hover:border-green-500",
    ],
    z: [
      "bg-blue-500/75 hover:bg-blue-500",
      "bg-gray-700/75 border-2 border-blue-500/75 hover:border-blue-500",
    ],
  };

  const rotations: Record<string, Vector3> = {
    "+x": new Vector3(0, 90, 0),
    "-x": new Vector3(0, -90, 0),
    "+y": new Vector3(90, 0, 0),
    "-y": new Vector3(-90, 0, 0),
    "+z": new Vector3(0, 0, 0),
    "-z": new Vector3(0, 180, 0),
  };

  return (
    <div className="relative">
      {Object.entries(axes).map(([axis, [colorCn, altColorCn]], i) => {
        const e = new Vector3().setComponent(i, 1).applyQuaternion(q);
        e.y *= -1;

        return [+1, -1].flatMap((sign) => {
          const angleRad = Math.atan2(e.y, e.x);
          const angledDistPx = distPx * Math.hypot(e.x, e.y) - diamPx / 2;
          const offsetPx = e.clone().multiplyScalar(sign * distPx);
          const signedAxis = `${sign > 0 ? "+" : "-"}${axis}`;

          const circle = (
            <button
              type="button"
              onClick={() => setRotation(rotations[signedAxis])}
              className={cn(
                "flex items-center justify-center rounded-full",
                sign > 0 ? colorCn : altColorCn
              )}
              style={{
                position: "absolute",
                width: diamPx,
                height: diamPx,
                zIndex: Math.round(baseZ * (1 + sign * e.z)),
                transform: `
                  translate(-50%, -50%)
                  translate(${offsetPx.x}px, ${offsetPx.y}px)
                `,
              }}
            >
              {sign > 0 && (
                <span className="text-gray-700 font-black text-xs">
                  {axis.toUpperCase()}
                </span>
              )}
            </button>
          );

          const line = angledDistPx > Number.EPSILON && (
            <div
              className={colorCn}
              style={{
                position: "absolute",
                width: angledDistPx,
                height: "2px",
                zIndex: Math.round(baseZ * (1 + (sign * e.z) / 2)),
                transform: `
                  translate(-50%, -50%)
                  rotate(${angleRad}rad)
                  translate(${sign * 50}%, 0)
                `,
              }}
            />
          );

          return sign > 0 ? (
            <Fragment key={signedAxis}>
              {circle}
              {line}
            </Fragment>
          ) : (
            <Fragment key={signedAxis}>
              {line}
              {circle}
            </Fragment>
          );
        });
      })}
    </div>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
