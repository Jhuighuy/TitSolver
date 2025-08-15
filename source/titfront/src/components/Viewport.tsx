/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { Spinner } from "@radix-ui/themes";
import { useEffect, useRef } from "react";

import { ColorBarWithTicks } from "~/components/ColorBar";
import { ViewCube } from "~/components/ViewCube";
import { useDataStore } from "~/stores/DataStore";
import { useCameraStore, useColorsStore } from "~/stores/ViewStore";
import { assert } from "~/utils";
import { backgroundColors } from "~/visual/BackroundColor";
import { Frame } from "~/visual/Frame";
import { Renderer } from "~/visual/Renderer";

import { Vector3 } from "three";

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

  // Helper function to execute a callback with the renderer, if it exists.
  function withRenderer(callback: (renderer: Renderer) => (() => void) | void) {
    const renderer = rendererRef.current;
    if (renderer) return callback(renderer);
  }

  // ---- Camera. --------------------------------------------------------------

  const {
    backgroundColor,
    projection: cameraProjection,
    position: cameraPosition,
    rotation: cameraRotation,
    zoom: cameraZoom,
    setPosition: setCameraPosition,
    setRotation: setCameraRotation,
    setZoom: setCameraZoom,
  } = useCameraStore();

  useEffect(() => {
    withRenderer((renderer) => {
      renderer.backgroundColor = backgroundColor;
    });
  }, [backgroundColor]);

  useEffect(() => {
    withRenderer((renderer) => {
      renderer.cameraController.camera.projection = cameraProjection;
      renderer.cameraController.camera.updateProjectionMatrix();
    });
  }, [cameraProjection]);

  useEffect(() => {
    withRenderer((renderer) => {
      renderer.cameraController.position.copy(cameraPosition);
    });
  }, [cameraPosition]);

  useEffect(() => {
    withRenderer((renderer) => {
      renderer.cameraController.rotation.setFromVector3(
        cameraRotation.clone().multiplyScalar(Math.PI / 180)
      );
    });
  }, [cameraRotation]);

  useEffect(() => {
    withRenderer((renderer) => {
      renderer.cameraController.camera.zoom = cameraZoom;
      renderer.cameraController.camera.updateProjectionMatrix();
    });
  }, [cameraZoom]);

  useEffect(() => {
    return withRenderer((renderer) => {
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
    });
  }, [setCameraPosition, setCameraRotation, setCameraZoom]);

  // ---- Colors. --------------------------------------------------------------

  const { colorMap, particleSize } = useColorsStore();

  useEffect(() => {
    withRenderer((renderer) => {
      renderer.setColorMap(colorMap);
    });
  }, [colorMap]);

  useEffect(() => {
    withRenderer((renderer) => {
      renderer.setParticleSize(particleSize);
    });
  }, [particleSize]);

  // ---- Data. ----------------------------------------------------------------

  const { requestedTimeStep, currentTimeStepData } = useDataStore();

  useEffect(() => {
    if (currentTimeStepData === null) return;
    withRenderer((renderer) => {
      renderer.addFrame(new Frame(currentTimeStepData));
    });
  }, [currentTimeStepData]);

  // ---- Layout. --------------------------------------------------------------

  return (
    <div className="size-full relative overflow-hidden bg-gradient-to-bl from-gray-700 to-gray-800">
      {/* ---- Top right: view cube. --------------------------------------- */}
      <div className="absolute top-12 right-12 z-10">
        <ViewCube rotation={cameraRotation} setRotation={setCameraRotation} />
      </div>

      {/* ---- Bottom left: color legend. ---------------------------------- */}
      <div className="absolute bottom-10 left-10 z-10">
        <ColorBarWithTicks
          colorMap={colorMap}
          min={0}
          max={100}
          widthPx={20}
          heightPx={300}
          dark={backgroundColors[backgroundColor].isDark}
        />
      </div>

      {/* ---- Center: loading indicator. ---------------------------------- */}
      {requestedTimeStep !== null && (
        <div className="absolute inset-0 z-10 flex items-center justify-center">
          <Spinner size="3" />
        </div>
      )}

      {/* ---- Canvas. ----------------------------------------------------- */}
      <canvas ref={canvasRef} className="absolute top-0 left-0" />
    </div>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
