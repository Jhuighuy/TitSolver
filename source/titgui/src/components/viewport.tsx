/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { Box, Flex, Select, Separator, Theme } from "@radix-ui/themes";
import { useEffect, useRef, useState } from "react";
import {
  TbBackground as BackgroundIcon,
  TbPalette as ColorMapIcon,
  TbGalaxy as FieldIcon,
  TbPerspectiveOff as OrthographicIcon,
  TbPerspective as PerspectiveIcon,
} from "react-icons/tb";
import { Vector3 } from "three";

import { TechText } from "~/components/basic";
import { ColorBox, ColorLegend } from "~/components/color-bar";
import { useStorage } from "~/components/storage";
import { ViewCube } from "~/components/view-cube";
import { assert, toCSSColor } from "~/utils";
import {
  type BackgroundColorName,
  backgroundColors,
} from "~/visual/background-color";
import type { Projection } from "~/visual/camera";
import { type ColorMapName, colorMaps } from "~/visual/color-map";
import { Renderer } from "~/visual/renderer";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export function Viewport() {
  const canvasRef = useRef<HTMLCanvasElement>(null);

  // ---- Renderer. ------------------------------------------------------------

  const rendererRef = useRef<Renderer | null>(null);
  const [colorMapName, setColorMapName] = useState<ColorMapName>("turbo");
  const [backgroundColorName, setBackgroundColorName] =
    useState<BackgroundColorName>("none");

  useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas) return;

    canvas.addEventListener("contextmenu", (event) => event.preventDefault());

    const renderer = new Renderer(canvas);
    rendererRef.current = renderer;

    const container = canvas.parentElement;
    assert(container !== null);
    renderer.resize(container.clientWidth, container.clientHeight);

    let resizeTimeout: ReturnType<typeof setTimeout> | null = null;
    const resizeObserver = new ResizeObserver(() => {
      if (resizeTimeout) clearTimeout(resizeTimeout);
      resizeTimeout = setTimeout(() => {
        renderer.resize(container.clientWidth, container.clientHeight);
      }, 100);
    });
    resizeObserver.observe(container, { box: "content-box" });

    return () => {
      resizeObserver.disconnect();
      rendererRef.current = null;
      renderer.dispose();
    };
  }, []);

  useEffect(() => {
    const renderer = rendererRef.current;
    if (renderer === null) return;

    renderer.particles.setColorMap(colorMapName);
  }, [colorMapName]);

  useEffect(() => {
    const renderer = rendererRef.current;
    if (renderer === null) return;

    renderer.setBackgroundColor(backgroundColorName);
  }, [backgroundColorName]);

  // ---- Fields. --------------------------------------------------------------

  const { frameData } = useStorage();
  const [currentField, setCurrentField] = useState("rho");

  useEffect(() => {
    if (frameData === null) return;

    const renderer = rendererRef.current;
    if (renderer === null) return;

    assert(currentField in frameData);
    renderer.particles.setData(frameData, currentField);
    renderer.particles.setColorRange(
      frameData[currentField].min,
      frameData[currentField].max
    );
  }, [frameData, currentField]);

  // ---- Camera. --------------------------------------------------------------

  const [projection, setProjection] = useState<Projection>("orthographic");
  const [cameraRotation, setCameraRotation] = useState(new Vector3(0, 0, 0));

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

    renderer.cameraController.camera.projection = projection;
    renderer.cameraController.camera.updateProjectionMatrix();
  }, [projection]);

  // ---- Layout. --------------------------------------------------------------

  return (
    <Flex direction="column" width="100%" height="100%" gap="1px">
      {/* ---- Controls. --------------------------------------------------- */}
      <Flex
        direction="row"
        align="center"
        height="36px"
        px="4"
        py="4px"
        gap="3"
        className="bg-linear-to-br from-gray-700 to-gray-800"
      >
        {/* ---- Projection. ----------------------------------------------- */}
        <Select.Root
          size="1"
          value={projection}
          onValueChange={(x) => setProjection(x as Projection)}
        >
          <Select.Trigger variant="ghost">
            <Flex align="center" gap="2">
              {projection === "perspective" ? (
                <PerspectiveIcon size={16} />
              ) : (
                <OrthographicIcon size={16} />
              )}
              Projection
            </Flex>
          </Select.Trigger>
          <Select.Content>
            <Select.Item value="perspective">
              <Flex align="center" gap="2">
                <PerspectiveIcon size={16} />
                Perspective
              </Flex>
            </Select.Item>
            <Select.Item value="orthographic">
              <Flex align="center" gap="2">
                <OrthographicIcon size={16} />
                Orthographic
              </Flex>
            </Select.Item>
          </Select.Content>
        </Select.Root>

        <Separator orientation="vertical" size="1" />

        {/* ---- Color map. ------------------------------------------------ */}
        <Select.Root
          size="1"
          value={colorMapName}
          onValueChange={(x) => setColorMapName(x as ColorMapName)}
        >
          <Select.Trigger variant="ghost">
            <Flex align="center" gap="2">
              <ColorMapIcon size={16} />
              Color map
            </Flex>
          </Select.Trigger>
          <Select.Content>
            {Object.entries(colorMaps).map(([name, { label }]) => (
              <Select.Item key={label} value={name}>
                <Flex align="center" gap="2">
                  <ColorBox
                    name={name as ColorMapName}
                    width="64px"
                    height="16px"
                  />
                  {label}
                </Flex>
              </Select.Item>
            ))}
          </Select.Content>
        </Select.Root>

        <Separator orientation="vertical" size="1" />

        {/* ---- Background color. ----------------------------------------- */}
        <Select.Root
          size="1"
          value={backgroundColorName}
          onValueChange={(x) =>
            setBackgroundColorName(x as BackgroundColorName)
          }
        >
          <Select.Trigger variant="ghost">
            <Flex align="center" gap="2">
              <BackgroundIcon size={16} />
              Background
            </Flex>
          </Select.Trigger>
          <Select.Content>
            {Object.entries(backgroundColors).map(
              ([name, { label, color }]) => (
                <Select.Item key={label} value={name}>
                  <Flex align="center" gap="2">
                    <Box
                      width="16px"
                      height="16px"
                      {...(color !== null && {
                        style: { backgroundColor: toCSSColor(color) },
                      })}
                    />
                    {label}
                  </Flex>
                </Select.Item>
              )
            )}
          </Select.Content>
        </Select.Root>

        <Separator orientation="vertical" size="1" />

        {/* ---- Field selection. ------------------------------------------ */}
        <Select.Root
          size="1"
          value={currentField}
          onValueChange={setCurrentField}
        >
          <Select.Trigger variant="ghost">
            <Flex align="center" gap="2">
              <FieldIcon size={16} />
              Field
            </Flex>
          </Select.Trigger>
          {frameData !== null && (
            <Select.Content>
              {Object.keys(frameData).map((name) => (
                <Select.Item key={name} value={name}>
                  <TechText>{name}</TechText>
                </Select.Item>
              ))}
            </Select.Content>
          )}
        </Select.Root>
      </Flex>

      {/* ---- Canvas. ----------------------------------------------------- */}
      <Box
        flexGrow="1"
        width="100%"
        height="100%"
        overflow="hidden"
        position="relative"
        className="bg-linear-to-bl from-gray-700 to-gray-800"
      >
        <Box position="absolute" asChild>
          <canvas ref={canvasRef} />
        </Box>

        <Theme appearance={backgroundColors[backgroundColorName].appearance}>
          {/* ---- View cube. ---------------------------------------------- */}
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

          {/* ---- Color legend. ------------------------------------------- */}
          {frameData !== null && (
            <Box
              position="absolute"
              left="8"
              top="50%"
              style={{ transform: "translate(-50%, -50%)" }}
            >
              <ColorLegend
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
    </Flex>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
