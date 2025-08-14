/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { Box, Select, Theme } from "@radix-ui/themes";
import { useEffect, useRef, useState } from "react";
import { Vector3 } from "three";

import { ColorLegendProps } from "~/components/color-bar";
import { useStorage } from "~/components/storage";
import { ViewCube } from "~/components/view-cube";
import { assert } from "~/utils";
import { backgroundColors } from "~/visual/backround-color";
import { Renderer } from "~/visual/renderer";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export function Viewport() {
  const canvasRef = useRef<HTMLCanvasElement>(null);

  /** @todo These should be configurable by the user. */
  const colorMapName = "jet";
  const backgroundColorName = "none";

  // ---- Renderer. ------------------------------------------------------------

  const rendererRef = useRef<Renderer | null>(null);

  useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas) return;

    const renderer = new Renderer(canvas);
    rendererRef.current = renderer;

    /** @todo This should be removed once the renderer is fully configurable. */
    renderer.setBackgroundColor(backgroundColorName);
    renderer.particles.setColorMap(colorMapName);

    const container = canvas.parentElement;
    assert(container !== null);
    renderer.resize(container.clientWidth, container.clientHeight);

    const resizeObserver = new ResizeObserver(() =>
      setTimeout(() => {
        renderer.resize(container.clientWidth, container.clientHeight);
      }, 100)
    );
    resizeObserver.observe(container, { box: "content-box" });

    return () => {
      resizeObserver.disconnect();
      rendererRef.current = null;
      renderer.dispose();
    };
  }, []);

  // ---- Fields. --------------------------------------------------------------

  const { frameData } = useStorage();
  const [currentField, setCurrentField] = useState("rho");

  useEffect(() => {
    if (frameData === null) return;

    const renderer = rendererRef.current;
    if (renderer === null) return;

    renderer.particles.setData(frameData, currentField);
    renderer.particles.setColorRange(
      frameData[currentField].min,
      frameData[currentField].max
    );
  }, [frameData, currentField]);

  // ---- Camera. --------------------------------------------------------------

  const [cameraRotation, setCameraRotation] = useState(new Vector3(0, 0, 0));

  useEffect(() => {
    const renderer = rendererRef.current;
    if (renderer === null) return;

    renderer.cameraController.rotation.setFromVector3(
      cameraRotation.clone().multiplyScalar(Math.PI / 180)
    );
  }, [cameraRotation]);

  useEffect(() => {
    const renderer = rendererRef.current;
    if (renderer === null) return;

    const controller = renderer.cameraController;

    function handleChange() {
      setCameraRotation(
        new Vector3().copy(controller.rotation).multiplyScalar(180 / Math.PI)
      );
    }

    controller.addEventListener("changed", handleChange);
    return () => controller.removeEventListener("changed", handleChange);
  }, []);

  // ---- Layout. --------------------------------------------------------------

  return (
    <Box
      width="100%"
      height="100%"
      overflow="hidden"
      position="relative"
      className="bg-linear-to-bl from-gray-700 to-gray-800"
    >
      {/* ---- Canvas. ----------------------------------------------------- */}
      <Box position="absolute" asChild>
        <canvas ref={canvasRef} />
      </Box>

      <Theme appearance={backgroundColors[backgroundColorName].appearance}>
        {/* ---- Field selector. ------------------------------------------- */}
        {/** @todo Should be removed once proper control is implemented. */}
        {frameData !== null && (
          <Box
            position="absolute"
            left="8"
            top="8"
            style={{ transform: "translate(-50%, -50%)" }}
          >
            <Select.Root value={currentField} onValueChange={setCurrentField}>
              <Select.Trigger />
              <Select.Content>
                {Object.keys(frameData).map((name) => (
                  <Select.Item key={name} value={name}>
                    {name}
                  </Select.Item>
                ))}
              </Select.Content>
            </Select.Root>
          </Box>
        )}

        {/* ---- View cube. ------------------------------------------------ */}
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

        {/* ---- Color legend. --------------------------------------------- */}
        {frameData !== null && (
          <Box
            position="absolute"
            left="8"
            top="50%"
            style={{ transform: "translate(-50%, -50%)" }}
          >
            <ColorLegendProps
              name={colorMapName}
              title={currentField}
              min={frameData[currentField].min}
              max={frameData[currentField].max}
              ticks={10}
              orientation="vertical"
              width="20px"
              height="500px"
            />
          </Box>
        )}
      </Theme>
    </Box>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
