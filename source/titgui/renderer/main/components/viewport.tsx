/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { useAtom, useAtomValue } from "jotai";
import { useEffect, useRef } from "react";

import { chrome } from "~/renderer/common/components/classes";
import { cn } from "~/renderer/common/components/utils";
import { Renderer } from "~/renderer/common/visual/renderer";
import { ViewColorLegend } from "~/renderer/main/components/view-color-legend";
import { ViewControls } from "~/renderer/main/components/view-controls";
import { ViewCube } from "~/renderer/main/components/view-cube";
import { ViewSelection } from "~/renderer/main/components/view-selection";
import {
  bindViewportRenderer,
  cameraRotationAtom,
  colorMapNameAtom,
  colorRangeAtom,
  colorTitleAtom,
  legendEnabledAtom,
  legendTickCountAtom,
} from "~/renderer/main/state/viewport";
import { assert } from "~/shared/utils";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export function Viewport() {
  // ---- Renderer. ------------------------------------------------------------

  const canvasRef = useRef<HTMLCanvasElement>(null);

  useEffect(() => {
    // Get canvas.
    const canvas = canvasRef.current;
    if (canvas === null) return;

    // Create renderer and bind it to the viewport state.
    const renderer = new Renderer(canvas);
    const unbindRenderer = bindViewportRenderer(renderer);

    // Keep renderer size synchronized with container size.
    const container = canvas.parentElement;
    assert(container !== null);
    renderer.resize(container.clientWidth, container.clientHeight);
    const resizeObserver = new ResizeObserver(() => {
      renderer.resize(container.clientWidth, container.clientHeight);
    });
    resizeObserver.observe(container, { box: "content-box" });

    return () => {
      // Dispose renderer and listeners.
      unbindRenderer();
      resizeObserver.disconnect();
      renderer.dispose();
    };
  }, []);

  // ---- State. ---------------------------------------------------------------

  const [cameraRotation, setCameraRotation] = useAtom(cameraRotationAtom);
  const legendEnabled = useAtomValue(legendEnabledAtom);
  const colorMapName = useAtomValue(colorMapNameAtom);
  const colorTitle = useAtomValue(colorTitleAtom);
  const colorRange = useAtomValue(colorRangeAtom);
  const legendTickCount = useAtomValue(legendTickCountAtom);

  // ---- Layout. --------------------------------------------------------------

  return (
    <div className={cn("flex size-full flex-col", chrome())}>
      {/* ---- Controls. --------------------------------------------------- */}
      <ViewControls />

      {/* ---- Viewport. ----------------------------------------------------*/}
      <ViewSelection>
        <canvas ref={canvasRef} style={{ position: "absolute" }} />

        <div className="absolute top-12 right-12 translate-x-1/2 -translate-y-1/2">
          <ViewCube
            className="size-24"
            rotation={cameraRotation}
            setRotation={setCameraRotation}
          />
        </div>

        {legendEnabled && (
          <div className="absolute top-1/2 left-12 -translate-x-1/2 -translate-y-1/2">
            <ViewColorLegend
              name={colorMapName}
              title={colorTitle}
              min={colorRange.min}
              max={colorRange.max}
              ticks={legendTickCount}
              className="h-125 w-5"
            />
          </div>
        )}
      </ViewSelection>
    </div>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
