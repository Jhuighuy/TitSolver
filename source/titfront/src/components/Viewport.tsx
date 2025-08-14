/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { Box, Spinner, Theme } from "@radix-ui/themes";
import { useEffect, useRef } from "react";
import { Vector3 } from "three";

import { ColorLegendProps } from "~/components/ColorBar";
import { ViewCube } from "~/components/ViewCube";
import { useDataStore } from "~/stores/DataStore";
import {
  useCameraStore,
  useColorLegendStore,
  useFieldsStore,
} from "~/stores/ViewStore";
import { assert } from "~/utils";
import { backgroundColors } from "~/visual/BackroundColor";
import { Frame } from "~/visual/Frame";
import { Renderer } from "~/visual/Renderer";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export function Viewport() {
  const canvasRef = useRef<HTMLCanvasElement>(null);

  // ---- Renderer. ------------------------------------------------------------

  const rendererRef = useRef<Renderer | null>(null);

  // Create renderer once canvas is available.
  useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas) return;

    // Create the renderer.
    const renderer = new Renderer(canvas);
    rendererRef.current = renderer;

    // Resize the renderer to match the container size.
    const container = canvas.parentElement;
    assert(container !== null);
    renderer.resize(container.clientWidth, container.clientHeight);

    // Tie canvas resizing to the container resize event.
    const resizeObserver = new ResizeObserver(() =>
      setTimeout(() => {
        renderer.resize(container.clientWidth, container.clientHeight);
      }, 100)
    );
    resizeObserver.observe(container, { box: "content-box" });

    return () => {
      rendererRef.current = null;
      renderer.dispose();
      resizeObserver.disconnect();
    };
  }, []);

  // ---- Fields. --------------------------------------------------------------

  const { requestedTimeStep, currentDataSet } = useDataStore();
  const {
    field,
    colorMapName,
    colorRangeType,
    colorRangeMin,
    colorRangeMax,
    setColorRange,
  } = useFieldsStore();

  useEffect(() => {
    const renderer = rendererRef.current;
    if (renderer === null) return;
    if (currentDataSet === null) return;
    renderer.addFrame(new Frame(currentDataSet, { colorField: field }));
  }, [currentDataSet, field]);

  useEffect(() => {
    if (colorRangeType !== "auto" || currentDataSet === null) return;
    setColorRange(currentDataSet[field].min, currentDataSet[field].max);
  }, [field, colorRangeType, currentDataSet, setColorRange]);

  useEffect(() => {
    const renderer = rendererRef.current;
    if (renderer === null) return;
    renderer.setColorMap(colorMapName);
  }, [colorMapName]);

  useEffect(() => {
    const renderer = rendererRef.current;
    if (renderer === null) return;
    renderer.setColorRange(colorRangeMin, colorRangeMax);
  }, [colorRangeMin, colorRangeMax]);

  // ---- Camera. --------------------------------------------------------------

  const {
    backgroundColorName,
    particleSize,
    projection: cameraProjection,
    position: cameraPosition,
    rotation: cameraRotation,
    zoom: cameraZoom,
    setPosition: setCameraPosition,
    setRotation: setCameraRotation,
    setZoom: setCameraZoom,
  } = useCameraStore();

  useEffect(() => {
    const renderer = rendererRef.current;
    if (renderer === null) return;
    renderer.setParticleSize(particleSize);
  }, [particleSize]);

  useEffect(() => {
    const renderer = rendererRef.current;
    if (renderer === null) return;
    renderer.backgroundColor = backgroundColorName;
  }, [backgroundColorName]);

  useEffect(() => {
    const renderer = rendererRef.current;
    if (renderer === null) return;
    renderer.cameraController.camera.projection = cameraProjection;
    renderer.cameraController.camera.updateProjectionMatrix();
  }, [cameraProjection]);

  useEffect(() => {
    const renderer = rendererRef.current;
    if (renderer === null) return;
    renderer.cameraController.position.copy(cameraPosition);
  }, [cameraPosition]);

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
    renderer.cameraController.camera.zoom = cameraZoom;
    renderer.cameraController.camera.updateProjectionMatrix();
  }, [cameraZoom]);

  useEffect(() => {
    const renderer = rendererRef.current;
    if (renderer === null) return;

    const controller = renderer.cameraController;

    function handleChange() {
      setCameraPosition(controller.position);
      setCameraRotation(
        new Vector3().copy(controller.rotation).multiplyScalar(180 / Math.PI)
      );
      setCameraZoom(controller.camera.zoom);
    }

    controller.addEventListener("changed", handleChange);
    return () => controller.removeEventListener("changed", handleChange);
  }, [setCameraPosition, setCameraRotation, setCameraZoom]);

  // ---- Legend. --------------------------------------------------------------

  const { legendLocation, legendNumTicks, title } = useColorLegendStore();

  // ---- Layout. --------------------------------------------------------------

  return (
    <Box
      width="100%"
      height="100%"
      overflow="hidden"
      position="relative"
      className="bg-gradient-to-bl from-gray-700 to-gray-800"
    >
      {/* ---- Canvas. ----------------------------------------------------- */}
      <Box position="absolute" asChild>
        <canvas ref={canvasRef} />
      </Box>

      {/* ---- HUD. -------------------------------------------------------- */}
      <Theme appearance={backgroundColors[backgroundColorName].appearance}>
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
        {legendLocation !== "none" && (
          <Box
            position="absolute"
            {...(legendLocation === "left"
              ? { left: "8", top: "50%" }
              : { left: "50%", bottom: "6" })}
            style={{ transform: "translate(-50%, -50%)" }}
          >
            <ColorLegendProps
              name={colorMapName}
              title={title ?? field}
              min={colorRangeMin}
              max={colorRangeMax}
              ticks={legendNumTicks}
              orientation={
                legendLocation === "left" ? "vertical" : "horizontal"
              }
              {...(legendLocation === "left"
                ? { width: "20px", height: "500px" }
                : { width: "500px", height: "20px" })}
            />
          </Box>
        )}

        {/* ---- Loading indicator. ---------------------------------------- */}
        {requestedTimeStep !== null && (
          <Box
            position="absolute"
            left="50%"
            top="50%"
            style={{ transform: "translate(-50%, -50%)" }}
          >
            <Spinner size="3" />
          </Box>
        )}
      </Theme>
    </Box>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
