/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { type FC, useEffect, useState } from "react";
import { Euler, Quaternion, Vector3 } from "three";

import { useViewer } from "~/components/Viewer";
import { cn } from "~/utils";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

type Axis = "x" | "y" | "z";
type AxisSpec = {
  unit: Vector3;
  colorCn: string;
  darkerColorCn: string;
  borderColorCn: string;
};
const axes: Record<Axis, AxisSpec> = {
  x: {
    unit: new Vector3(+1, 0, 0),
    colorCn: "bg-red-500/50 hover:bg-red-500/75",
    darkerColorCn: "bg-red-500/25 hover:bg-red-500/50",
    borderColorCn: "border-red-500/50",
  },
  y: {
    unit: new Vector3(0, +1, 0),
    colorCn: "bg-green-500/50 hover:bg-green-500/75",
    darkerColorCn: "bg-green-500/25 hover:bg-green-500/50",
    borderColorCn: "border-green-500/50",
  },
  z: {
    unit: new Vector3(0, 0, -1),
    colorCn: "bg-blue-500/50 hover:bg-blue-500/75",
    darkerColorCn: "bg-blue-500/25 hover:bg-blue-500/50",
    borderColorCn: "border-blue-500/50",
  },
};

const baseZ = 20;
const diamPx = 30;
const distPx = 45;

export const Orientation: FC = () => {
  const viewer = useViewer();
  const [rotation, setRotation] = useState<Euler | null>(null);

  useEffect(() => {
    const controls = viewer?.controls;
    if (!controls) return;
    const setRotationFromControls = () =>
      setRotation(
        new Euler(
          viewer.controls.getPolarAngle() - Math.PI / 2,
          viewer.controls.getAzimuthalAngle(),
          0
        )
      );
    setRotationFromControls();
    controls.addEventListener("change", setRotationFromControls);
    return () => {
      controls.removeEventListener("change", setRotationFromControls);
    };
  }, [viewer]);

  if (!viewer || !rotation) return null;

  const q = new Quaternion().setFromEuler(rotation);

  return (
    <div className="absolute top-20 right-20">
      <OrientAxis axis="x" q={q} />
      <OrientAxis axis="y" q={q} />
      <OrientAxis axis="z" q={q} />
    </div>
  );
};

const OrientAxis = ({ axis, q }: { axis: Axis; q: Quaternion }) => {
  const { unit } = axes[axis];
  const e = new Vector3().copy(unit).applyQuaternion(q);
  e.y *= -1;
  return (
    <>
      <Head axis={axis} sign={+1} e={e} />
      <Line axis={axis} sign={+1} e={e} />
      <Line axis={axis} sign={-1} e={e} />
      <Head axis={axis} sign={-1} e={e} />
    </>
  );
};

const Head = ({ axis, sign, e }: { axis: Axis; sign: number; e: Vector3 }) => {
  const { colorCn, darkerColorCn, borderColorCn } = axes[axis];
  return (
    <button
      type="button"
      className={cn(
        "absolute flex items-center justify-center",
        "rounded-full",
        sign > 0
          ? cn("text-gray-700 font-black", colorCn)
          : cn("border-4", darkerColorCn, borderColorCn)
      )}
      style={{
        width: diamPx,
        height: diamPx,
        zIndex: Math.round(baseZ * (1 - sign * e.z)),
        transform: `
          translate(-50%, -50%)
          translate(${sign * e.x * distPx}px, ${sign * e.y * distPx}px)
        `,
      }}
    >
      {sign > 0 ? axis.toUpperCase() : ""}
    </button>
  );
};

const Line = ({ axis, sign, e }: { axis: Axis; sign: number; e: Vector3 }) => {
  const { colorCn } = axes[axis];
  const angleRad = Math.atan2(e.y, e.x);
  const angledDistPx = distPx * Math.hypot(e.x, e.y) - diamPx / 2;
  return (
    Math.abs(angledDistPx) > 2 && (
      <div
        className={cn("absolute", colorCn)}
        style={{
          width: angledDistPx,
          height: 4,
          zIndex: Math.round(baseZ * (1 - sign * 0.5 * e.z)),
          transform: `
            translate(-50%, -50%)
            rotate(${angleRad}rad)
            translate(${sign * 50}%, 0)
          `,
        }}
      />
    )
  );
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
