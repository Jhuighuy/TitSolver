/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import * as THREE from "three";
import React, { useEffect, useRef } from "react";

import { useTitView } from "../../TitViewProvider";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

const Orientation = ({ rotation }: { rotation?: THREE.Euler }) => {
  if (!rotation) return null;

  const q = new THREE.Quaternion();
  q.setFromEuler(rotation);
  q.invert();

  const OrientAxis = (axis: "x" | "y" | "z") => {
    const [lightBgColor, bgColor] = {
      x: ["bg-red-100", "bg-red-200 hover:bg-red-300"],
      y: ["bg-green-100", "bg-green-200 hover:bg-green-300"],
      z: ["bg-blue-100", "bg-blue-200 hover:bg-blue-300"],
    }[axis];

    const borderColor = {
      x: "border-red-200 hover:border-red-300",
      y: "border-green-200 hover:border-green-300",
      z: "border-blue-200 hover:border-blue-300",
    }[axis];

    let e = {
      x: new THREE.Vector3(+1, 0, 0),
      y: new THREE.Vector3(0, -1, 0),
      z: new THREE.Vector3(0, 0, -1),
    }[axis];

    e = e.applyQuaternion(q);

    const diam = 30; // Diameter of the axis circle, in pixels.
    const dist = 45; // Distance from origin to axis, in pixels.

    // Calculate the angle of the axis and the transformed distance.
    const angle = Math.atan2(e.y, e.x);
    const angledDist = dist * Math.hypot(e.x, e.y) - diam / 2;

    // Calculate depth of the axis.
    const baseZ = 20;
    const posCircleZ = Math.round(baseZ * (1 - e.z));
    const posLineZ = Math.round(baseZ * (1 - 0.5 * e.z));
    const negLineZ = Math.round(baseZ * (1 + 0.5 * e.z));
    const negCircleZ = Math.round(baseZ * (1 + e.z));

    return (
      <>
        {/* Positive direction circle. */}
        <div
          className={`
            absolute
            flex items-center justify-center
            rounded-full
            shadow
            ${bgColor}
            font-black text-gray-400
          `}
          style={{
            width: diam,
            height: diam,
            zIndex: posCircleZ,
            transform: `
              translate(-50%, -50%)
              translate(${e.x * dist}px, ${e.y * dist}px)
            `,
          }}
          onClick={() => rotation.set(0, 0, 0)}
        >
          {axis.toUpperCase()}
        </div>
        {/* Line from origin to positive direction. */}
        <div
          className={`
            absolute
            shadow
            ${bgColor}
          `}
          style={{
            width: angledDist,
            height: 4,
            zIndex: posLineZ,
            transform: `
              translate(-50%, -50%)
              rotate(${angle}rad)
              translate(50%, 0)
            `,
          }}
        />
        {/* Line from origin to negative direction. */}
        <div
          className={`
            absolute
            shadow
            ${bgColor}
          `}
          style={{
            width: angledDist,
            height: 4,
            zIndex: negLineZ,
            transform: `
              translate(-50%, -50%)
              rotate(${angle}rad)
              translate(-50%, 0)
            `,
          }}
        />
        {/* Negative direction circle. */}
        <div
          className={`
            absolute
            shadow
            rounded-full
            ${lightBgColor}
            border-4 ${borderColor}
          `}
          style={{
            width: diam,
            height: diam,
            zIndex: negCircleZ,
            transform: `
              translate(-50%, -50%)
              translate(${-e.x * dist}px, ${-e.y * dist}px)
            `,
          }}
        />
      </>
    );
  };

  return (
    <div className="absolute top-20 left-20">
      {OrientAxis("x")}
      {OrientAxis("y")}
      {OrientAxis("z")}
    </div>
  );
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

const Canvas: React.FC = () => {
  const titView = useTitView();
  const canvasRef = useRef<HTMLCanvasElement | null>(null);
  useEffect(() => {
    if (canvasRef.current === null) {
      throw new Error("Canvas element is not initialized!");
    }
    titView.canvas = canvasRef.current;
    /** @todo Use the actual canvas size. */
    titView.initializeRenderer(800.0, 600.0);

    // Start the rendering loop.
    let lastFrameTime = performance.now();
    let animationFrameID: number;
    const renderLoop = (currentTime: number) => {
      const deltaTime = (currentTime - lastFrameTime) / 1000;
      titView.renderFrame(deltaTime);
      lastFrameTime = currentTime;
      animationFrameID = requestAnimationFrame(renderLoop);
    };
    renderLoop(performance.now());

    return () => cancelAnimationFrame(animationFrameID);
  }, [titView, canvasRef]);

  return <canvas ref={canvasRef} />;
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export const MyCanvas: React.FC = () => {
  return (
    <div className="w-[99.99%] h-[99.99%]">
      <Orientation rotation={new THREE.Euler()} />
      <Canvas />
    </div>
  );
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
