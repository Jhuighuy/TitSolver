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
import { useViewportState } from "~/renderer/main/state/viewport";
import { assert } from "~/shared/utils";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export function Viewport() {
  const viewport = useViewportState();

  // ---- Renderer. ------------------------------------------------------------

  const canvasRef = useRef<HTMLCanvasElement>(null);

  const { bindRenderer } = viewport;
  useEffect(() => {
    // Get canvas.
    const canvas = canvasRef.current;
    if (canvas === null) return;

    // Create renderer and bind it to the viewport state.
    const renderer = new Renderer(canvas);
    const unbindRenderer = bindRenderer(renderer);

    // Keep renderer size synchronized with container size. A hidden
    // viewport (e.g. an inactive, kept-mounted tab) measures 0×0 — skip
    // those updates and resync once it becomes visible again.
    const container = canvas.parentElement;
    assert(container !== null);
    const resize = () => {
      const { clientWidth, clientHeight } = container;
      if (clientWidth === 0 || clientHeight === 0) return;
      renderer.resize(clientWidth, clientHeight);
    };
    resize();
    const resizeObserver = new ResizeObserver(resize);
    resizeObserver.observe(container, { box: "content-box" });

    return () => {
      // Dispose renderer and listeners.
      unbindRenderer();
      resizeObserver.disconnect();
      renderer.dispose();
    };
  }, [bindRenderer]);

  // ---- State. ---------------------------------------------------------------

  const [cameraRotation, setCameraRotation] = useAtom(
    viewport.cameraRotationAtom,
  );
  const legendEnabled = useAtomValue(viewport.legendEnabledAtom);
  const colorMapName = useAtomValue(viewport.colorMapNameAtom);
  const colorTitle = useAtomValue(viewport.colorTitleAtom);
  const colorRange = useAtomValue(viewport.colorRangeAtom);
  const legendTickCount = useAtomValue(viewport.legendTickCountAtom);

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
