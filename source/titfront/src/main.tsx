/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import React, { useEffect, useRef } from "react";
import ReactDOM from "react-dom/client";

import TitViewProvider, { useTitView } from "./TitViewProvider";
import "./index.css";

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

ReactDOM.createRoot(document.getElementById("root") as HTMLElement).render(
  <React.StrictMode>
    <TitViewProvider>
      <Canvas />
    </TitViewProvider>
  </React.StrictMode>
);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
