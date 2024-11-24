/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/**
 * TitView WASM module interface.
 */
export interface Module {
  /**
   * Canvas element.
   */
  canvas: HTMLCanvasElement | null;

  /**
   * Initialize the renderer.
   *
   * @param width  Canvas width in pixels.
   * @param height Canvas height in pixels.
   * @returns `1` on success, `-1` on failure.
   */
  initializeRenderer(width: number, height: number): number;

  /**
   * Render a frame.
   *
   * @param deltaTime Time delta in seconds.
   */
  renderFrame(deltaTime: number): void;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export default function (): Promise<Module>;
