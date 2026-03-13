/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import {
  DepthTexture,
  DepthStencilFormat,
  NearestFilter,
  RGBAFormat,
  Scene,
  UnsignedInt248Type,
  Vector2,
  WebGLRenderTarget,
  WebGLRenderer,
  type Camera as ThreeCamera,
} from "three";
import { EffectComposer } from "three/examples/jsm/postprocessing/EffectComposer.js";
import { ShaderPass } from "three/examples/jsm/postprocessing/ShaderPass.js";
import { SMAAPass } from "three/examples/jsm/postprocessing/SMAAPass.js";
import { TexturePass } from "three/examples/jsm/postprocessing/TexturePass.js";

import type { Camera } from "~/visual/camera";
import type { EffectsSettings } from "~/visual/effects";
import { edlShader } from "~/visual/post-effects/edl-shader";
import { ssaoBlurShader } from "~/visual/post-effects/ssao-blur-shader";
import { ssaoCompositeShader } from "~/visual/post-effects/ssao-composite-shader";
import { ssaoFactorShader } from "~/visual/post-effects/ssao-factor-shader";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
 * The renderer uses a dedicated scene render target so all depth-based effects
 * sample the original scene depth instead of the composer's ping-pong buffers.
 *
 * Composer order:
 * 1. `TexturePass` seeds the pipeline with the raw scene color.
 * 2. `SSAO factor` computes a monochrome AO term from depth.
 * 3. `SSAO blur X/Y` smooth that AO term while respecting depth edges.
 * 4. `SSAO composite` multiplies the blurred AO back into the original color.
 * 5. `EDL` darkens silhouettes and depth discontinuities.
 * 6. `SMAA` smooths the final composed image.
 */
export class PostProcessingPipeline {
  private readonly renderer: WebGLRenderer;
  private readonly sceneTarget: WebGLRenderTarget;
  private readonly sceneDepthTexture: DepthTexture;
  private readonly composer: EffectComposer;
  private readonly sourcePass: TexturePass;
  private readonly ssaoFactorPass: ShaderPass;
  private readonly ssaoBlurHorizontalPass: ShaderPass;
  private readonly ssaoBlurVerticalPass: ShaderPass;
  private readonly ssaoCompositePass: ShaderPass;
  private readonly edlPass: ShaderPass;
  private readonly smaaPass: SMAAPass;
  private readonly resolution = new Vector2(1, 1);

  public constructor(renderer: WebGLRenderer) {
    this.renderer = renderer;

    // Render the scene once into an offscreen target with a sampleable
    // depth-stencil texture. SSAO and EDL both read from this depth texture.
    this.sceneTarget = new WebGLRenderTarget(1, 1, {
      depthBuffer: true,
      stencilBuffer: true,
    });
    this.sceneTarget.texture.format = RGBAFormat;
    this.sceneTarget.texture.minFilter = NearestFilter;
    this.sceneTarget.texture.magFilter = NearestFilter;

    this.sceneDepthTexture = new DepthTexture(1, 1, UnsignedInt248Type);
    this.sceneDepthTexture.format = DepthStencilFormat;
    this.sceneDepthTexture.minFilter = NearestFilter;
    this.sceneDepthTexture.magFilter = NearestFilter;
    this.sceneTarget.depthTexture = this.sceneDepthTexture;
    this.sceneTarget.samples = 0;

    // The composer owns the post stack, but its read/write buffers only carry
    // color. The original scene depth stays attached to `sceneTarget`.
    this.composer = new EffectComposer(renderer);

    // Seed the composer with the scene color produced by the offscreen render.
    this.sourcePass = new TexturePass(this.sceneTarget.texture);
    this.sourcePass.clear = true;

    // Generate a scalar AO term from depth and reconstructed normals.
    this.ssaoFactorPass = new ShaderPass(ssaoFactorShader);
    this.ssaoFactorPass.uniforms.depthTexture.value = this.sceneDepthTexture;
    this.ssaoFactorPass.uniforms.resolution.value = this.resolution;
    this.ssaoFactorPass.enabled = false;

    // Bilateral-style separable blur: horizontal then vertical.
    this.ssaoBlurHorizontalPass = new ShaderPass(ssaoBlurShader);
    this.ssaoBlurHorizontalPass.uniforms.depthTexture.value =
      this.sceneDepthTexture;
    this.ssaoBlurHorizontalPass.uniforms.resolution.value = this.resolution;
    this.ssaoBlurHorizontalPass.uniforms.direction.value = new Vector2(1, 0);
    this.ssaoBlurHorizontalPass.enabled = false;

    this.ssaoBlurVerticalPass = new ShaderPass(ssaoBlurShader);
    this.ssaoBlurVerticalPass.uniforms.depthTexture.value = this.sceneDepthTexture;
    this.ssaoBlurVerticalPass.uniforms.resolution.value = this.resolution;
    this.ssaoBlurVerticalPass.uniforms.direction.value = new Vector2(0, 1);
    this.ssaoBlurVerticalPass.enabled = false;

    // Re-apply the blurred AO factor to the original scene color.
    this.ssaoCompositePass = new ShaderPass(ssaoCompositeShader);
    this.ssaoCompositePass.uniforms.colorTexture.value = this.sceneTarget.texture;
    this.ssaoCompositePass.enabled = false;

    // EDL operates on the already AO-composited color but still samples the
    // original scene depth.
    this.edlPass = new ShaderPass(edlShader);
    this.edlPass.uniforms.depthTexture.value = this.sceneDepthTexture;
    this.edlPass.uniforms.resolution.value = this.resolution;
    this.edlPass.enabled = true;

    this.smaaPass = new SMAAPass();
    this.smaaPass.enabled = true;

    // The last enabled pass automatically renders to screen.
    this.composer.addPass(this.sourcePass);
    this.composer.addPass(this.ssaoFactorPass);
    this.composer.addPass(this.ssaoBlurHorizontalPass);
    this.composer.addPass(this.ssaoBlurVerticalPass);
    this.composer.addPass(this.ssaoCompositePass);
    this.composer.addPass(this.edlPass);
    this.composer.addPass(this.smaaPass);
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  public dispose() {
    this.composer.dispose();
    this.sceneTarget.dispose();
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  public resize(width: number, height: number) {
    // Keep all resolution-dependent uniforms and textures in sync.
    this.resolution.set(width, height);
    this.sceneTarget.setSize(width, height);
    this.sceneDepthTexture.image.width = width;
    this.sceneDepthTexture.image.height = height;
    this.composer.setSize(width, height);
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  public setSettings(settings: EffectsSettings) {
    // SSAO is represented by three physical passes in the composer, so all
    // of them must be toggled together.
    this.ssaoFactorPass.enabled = settings.ssao.enabled;
    this.ssaoBlurHorizontalPass.enabled = settings.ssao.enabled;
    this.ssaoBlurVerticalPass.enabled = settings.ssao.enabled;
    this.ssaoCompositePass.enabled = settings.ssao.enabled;

    this.ssaoFactorPass.uniforms.radius.value = settings.ssao.radius;
    this.ssaoFactorPass.uniforms.intensity.value = settings.ssao.intensity;
    this.ssaoFactorPass.uniforms.bias.value = settings.ssao.bias;

    this.edlPass.enabled = settings.edl.enabled;
    this.edlPass.uniforms.radius.value = settings.edl.radius;
    this.edlPass.uniforms.strength.value = settings.edl.strength;

    this.smaaPass.enabled = settings.smaa.enabled;
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  public render(scene: Scene, camera: Camera) {
    const threeCamera = camera as unknown as ThreeCamera;

    // Camera-dependent uniforms are refreshed every frame because both the
    // projection mode and camera transform can change interactively.
    this.ssaoFactorPass.uniforms.cameraNear.value = camera.near;
    this.ssaoFactorPass.uniforms.cameraFar.value = camera.far;
    this.ssaoFactorPass.uniforms.isPerspective.value =
      camera.projection === "perspective" ? 1 : 0;
    this.ssaoFactorPass.uniforms.inverseProjectionMatrix.value =
      camera.projectionMatrixInverse;

    this.ssaoBlurHorizontalPass.uniforms.cameraNear.value = camera.near;
    this.ssaoBlurHorizontalPass.uniforms.cameraFar.value = camera.far;
    this.ssaoBlurHorizontalPass.uniforms.isPerspective.value =
      camera.projection === "perspective" ? 1 : 0;

    this.ssaoBlurVerticalPass.uniforms.cameraNear.value = camera.near;
    this.ssaoBlurVerticalPass.uniforms.cameraFar.value = camera.far;
    this.ssaoBlurVerticalPass.uniforms.isPerspective.value =
      camera.projection === "perspective" ? 1 : 0;

    this.edlPass.uniforms.cameraNear.value = camera.near;
    this.edlPass.uniforms.cameraFar.value = camera.far;
    this.edlPass.uniforms.isPerspective.value =
      camera.projection === "perspective" ? 1 : 0;

    // First render the scene and its depth into the dedicated source target.
    this.renderer.setRenderTarget(this.sceneTarget);
    this.renderer.clear(true, true, true);
    this.renderer.render(scene, threeCamera);

    // Then run the post stack using the scene color as the initial input.
    this.sourcePass.map = this.sceneTarget.texture;
    this.ssaoCompositePass.uniforms.colorTexture.value = this.sceneTarget.texture;
    this.composer.render();
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
