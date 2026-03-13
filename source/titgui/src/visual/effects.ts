/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import {
  DepthFormat,
  DepthTexture,
  NearestFilter,
  RGBAFormat,
  type Scene,
  type Texture,
  UnsignedIntType,
  type WebGLRenderer,
  WebGLRenderTarget,
} from "three";
import { EffectComposer } from "three/examples/jsm/postprocessing/EffectComposer.js";
import { SMAAPass } from "three/examples/jsm/postprocessing/SMAAPass.js";
import { TexturePass } from "three/examples/jsm/postprocessing/TexturePass.js";

import type { Camera } from "~/visual/camera";
import { DepthPass } from "~/visual/passes/depth";
import { DepthCuePass } from "~/visual/passes/depth-cue";
import { EDLPass } from "~/visual/passes/edl";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export type EDLSettings = {
  enabled: boolean;
  radius: number;
  strength: number;
};

export type DepthCueSettings = {
  enabled: boolean;
  strength: number;
  exponent: number;
};

export type SMAASettings = {
  enabled: boolean;
};

export type EffectsSettings = {
  edl: EDLSettings;
  depthCue: DepthCueSettings;
  smaa: SMAASettings;
};

export const effectsSettingsDefault: EffectsSettings = {
  edl: {
    enabled: true,
    radius: 1.5,
    strength: 1.2,
  },
  depthCue: {
    enabled: false,
    strength: 0.35,
    exponent: 1.5,
  },
  smaa: {
    enabled: true,
  },
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
 * Owns the renderer's post-processing graph.
 */
export class EffectsPipeline {
  private readonly renderer: WebGLRenderer;
  private readonly sceneColorTarget: WebGLRenderTarget;
  private readonly depthPass: DepthPass;
  private readonly edlPass: EDLPass;
  private readonly depthCuePass: DepthCuePass;
  private readonly composer: EffectComposer;
  private readonly sourcePass: TexturePass;
  private readonly smaaPass: SMAAPass;
  private settings: EffectsSettings | null = null;

  public constructor(renderer: WebGLRenderer) {
    this.renderer = renderer;

    this.sceneColorTarget = new WebGLRenderTarget(1, 1, {
      depthBuffer: true,
      stencilBuffer: false,
    });
    this.sceneColorTarget.texture.format = RGBAFormat;
    this.sceneColorTarget.texture.minFilter = NearestFilter;
    this.sceneColorTarget.texture.magFilter = NearestFilter;
    this.sceneColorTarget.samples = 0;
    const sceneDepthTexture = new DepthTexture(1, 1, UnsignedIntType);
    sceneDepthTexture.format = DepthFormat;
    sceneDepthTexture.minFilter = NearestFilter;
    sceneDepthTexture.magFilter = NearestFilter;
    this.sceneColorTarget.depthTexture = sceneDepthTexture;

    this.depthPass = new DepthPass();
    this.edlPass = new EDLPass();
    this.depthCuePass = new DepthCuePass();

    this.composer = new EffectComposer(renderer);
    this.sourcePass = new TexturePass(this.sceneColorTarget.texture);
    this.sourcePass.clear = true;
    this.smaaPass = new SMAAPass();
    this.smaaPass.enabled = true;
    this.composer.addPass(this.sourcePass);
    this.composer.addPass(this.smaaPass);
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  public dispose() {
    this.composer.dispose();
    this.sceneColorTarget.depthTexture?.dispose();
    this.sceneColorTarget.dispose();
    this.depthPass.dispose();
    this.edlPass.dispose();
    this.depthCuePass.dispose();
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  public resize(width: number, height: number) {
    this.sceneColorTarget.setSize(width, height);
    const rawDepthTexture = this.sceneColorTarget.depthTexture;
    if (rawDepthTexture !== null) {
      rawDepthTexture.image.width = width;
      rawDepthTexture.image.height = height;
    }
    this.depthPass.resize(width, height);
    this.edlPass.resize(width, height);
    this.depthCuePass.resize(width, height);
    this.composer.setSize(width, height);
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  public setSettings(settings: EffectsSettings) {
    this.settings = settings;
    this.edlPass.setSettings(settings.edl.radius, settings.edl.strength);
    this.depthCuePass.setSettings(
      settings.depthCue.strength,
      settings.depthCue.exponent,
    );
    this.smaaPass.enabled = settings.smaa.enabled;
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  public render(scene: Scene, camera: Camera) {
    const settings = this.settings;
    if (settings === null) return;

    this.renderer.setRenderTarget(this.sceneColorTarget);
    this.renderer.clear(true, true, true);
    this.renderer.render(scene, camera);

    const rawDepthTexture = this.sceneColorTarget.depthTexture;
    if (rawDepthTexture === null) {
      throw new Error("Scene color target is missing its depth texture.");
    }

    let currentColorTexture: Texture = this.sceneColorTarget.texture;
    const needsLinearDepth = settings.edl.enabled || settings.depthCue.enabled;
    if (needsLinearDepth) {
      this.depthPass.renderPass(this.renderer, camera, rawDepthTexture);
    }

    if (settings.edl.enabled) {
      this.edlPass.renderPass(
        this.renderer,
        camera,
        currentColorTexture,
        this.depthPass.texture,
      );
      currentColorTexture = this.edlPass.texture;
    }

    if (settings.depthCue.enabled) {
      this.depthCuePass.renderPass(
        this.renderer,
        camera,
        currentColorTexture,
        this.depthPass.texture,
      );
      currentColorTexture = this.depthCuePass.texture;
    }

    if (settings.smaa.enabled) {
      this.sourcePass.map = currentColorTexture;
      this.composer.render();
      return;
    }

    this.sourcePass.map = currentColorTexture;
    this.sourcePass.renderToScreen = true;
    this.sourcePass.render(
      this.renderer,
      this.sceneColorTarget,
      this.sceneColorTarget,
      0,
      false,
    );
    this.sourcePass.renderToScreen = false;
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
