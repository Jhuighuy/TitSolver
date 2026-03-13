/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import {
  NearestFilter,
  type PixelFormat,
  RGBAFormat,
  ShaderMaterial,
  type Texture,
  type TextureDataType,
  type WebGLRenderer,
  WebGLRenderTarget,
} from "three";
import {
  FullScreenQuad,
  Pass,
} from "three/examples/jsm/postprocessing/Pass.js";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

type FullscreenPassOptions = {
  uniforms: Record<string, { value: unknown }>;
  vertexShader: string;
  fragmentShader: string;
  format?: PixelFormat;
  type?: TextureDataType;
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
 * Shared scaffolding for our custom fullscreen render passes.
 *
 * The renderer still orchestrates the pass graph manually, but the pass
 * implementation details now reuse Three's post-processing primitives instead
 * of each pass building its own fullscreen scene and quad from scratch.
 */
export class FullscreenPass extends Pass {
  protected readonly material: ShaderMaterial;
  private readonly target: WebGLRenderTarget;
  private readonly quad: FullScreenQuad;

  public constructor({
    uniforms,
    vertexShader,
    fragmentShader,
    format = RGBAFormat,
    type,
  }: FullscreenPassOptions) {
    super();

    this.target = new WebGLRenderTarget(1, 1, {
      format,
      ...(type === undefined ? {} : { type }),
      depthBuffer: false,
      stencilBuffer: false,
    });
    this.target.texture.minFilter = NearestFilter;
    this.target.texture.magFilter = NearestFilter;

    this.material = new ShaderMaterial({
      depthTest: false,
      depthWrite: false,
      uniforms,
      vertexShader,
      fragmentShader,
    });

    this.quad = new FullScreenQuad(this.material);
    this.needsSwap = false;
    this.clear = true;
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  public get texture(): Texture {
    return this.target.texture;
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  public override dispose() {
    this.target.dispose();
    this.material.dispose();
    this.quad.dispose();
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  public override setSize(width: number, height: number) {
    this.target.setSize(width, height);
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  protected renderToTarget(renderer: WebGLRenderer) {
    renderer.setRenderTarget(this.renderToScreen ? null : this.target);
    if (this.clear) renderer.clear(true, true, true);
    this.quad.render(renderer);
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  public override render(renderer: WebGLRenderer) {
    this.renderToTarget(renderer);
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
