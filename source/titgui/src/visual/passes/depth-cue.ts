/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import {
  FloatType,
  RGBAFormat,
  type Texture,
  type WebGLRenderer,
} from "three";

import type { Camera } from "~/visual/camera";
import { FullscreenPass } from "~/visual/passes/fullscreen-pass";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
 * Applies a simple depth cueing term so distant particles read more softly
 * than the near surface envelope.
 */
export class DepthCuePass extends FullscreenPass {
  public constructor() {
    super({
      format: RGBAFormat,
      type: FloatType,
      uniforms: {
        colorTexture: { value: null },
        depthTexture: { value: null },
        cameraNear: { value: 0.01 },
        cameraFar: { value: 1000.0 },
        strength: { value: 0.35 },
        exponent: { value: 1.5 },
      },
      vertexShader: `
        varying vec2 vUv;

        void main() {
          vUv = uv;
          gl_Position = vec4(position.xy, 0.0, 1.0);
        }
      `,
      fragmentShader: `
        uniform sampler2D colorTexture;
        uniform sampler2D depthTexture;
        uniform float cameraNear;
        uniform float cameraFar;
        uniform float strength;
        uniform float exponent;

        varying vec2 vUv;

        void main() {
          vec4 baseColor = texture2D(colorTexture, vUv);
          vec4 depth = texture2D(depthTexture, vUv);
          if (baseColor.a <= 0.0 || depth.a <= 0.0) {
            gl_FragColor = baseColor;
            return;
          }

          float normalizedDepth = clamp(
            (depth.r - cameraNear) / max(cameraFar - cameraNear, 1e-6),
            0.0,
            1.0
          );
          float cue = strength * pow(normalizedDepth, exponent);
          gl_FragColor = vec4(baseColor.rgb * (1.0 - cue), baseColor.a);
        }
      `,
    });
  }

  public resize(width: number, height: number) {
    this.setSize(width, height);
  }

  public setSettings(strength: number, exponent: number) {
    this.material.uniforms.strength.value = strength;
    this.material.uniforms.exponent.value = exponent;
  }

  public renderPass(
    renderer: WebGLRenderer,
    camera: Camera,
    colorTexture: Texture,
    depthTexture: Texture,
  ) {
    this.material.uniforms.colorTexture.value = colorTexture;
    this.material.uniforms.depthTexture.value = depthTexture;
    this.material.uniforms.cameraNear.value = camera.near;
    this.material.uniforms.cameraFar.value = camera.far;

    this.renderToTarget(renderer);
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
