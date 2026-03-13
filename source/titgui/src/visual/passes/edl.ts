/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import {
  FloatType,
  RGBAFormat,
  type Texture,
  Vector2,
  type WebGLRenderer,
} from "three";

import type { Camera } from "~/visual/camera";
import { FullscreenPass } from "~/visual/passes/fullscreen-pass";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
 * Applies Eye-Dome Lighting using the already composed color image plus the
 * shared linear depth image.
 */
export class EDLPass extends FullscreenPass {
  public constructor() {
    super({
      format: RGBAFormat,
      type: FloatType,
      uniforms: {
        colorTexture: { value: null },
        depthTexture: { value: null },
        resolution: { value: new Vector2(1, 1) },
        radius: { value: 1.5 },
        strength: { value: 1.2 },
        cameraFar: { value: 10 },
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
        uniform vec2 resolution;
        uniform float radius;
        uniform float strength;
        uniform float cameraFar;

        varying vec2 vUv;

        void main() {
          vec4 baseColor = texture2D(colorTexture, vUv);
          vec4 centerDepth = texture2D(depthTexture, vUv);
          if (baseColor.a <= 0.0 || centerDepth.a <= 0.0) {
            gl_FragColor = baseColor;
            return;
          }

          float centerLogDepth = log2(max(centerDepth.r, 1e-6));
          vec2 texel = radius / resolution;
          float response = 0.0;

          vec2 offsets[8];
          offsets[0] = vec2( 1.0,  0.0);
          offsets[1] = vec2(-1.0,  0.0);
          offsets[2] = vec2( 0.0,  1.0);
          offsets[3] = vec2( 0.0, -1.0);
          offsets[4] = vec2( 0.70710678,  0.70710678);
          offsets[5] = vec2(-0.70710678,  0.70710678);
          offsets[6] = vec2( 0.70710678, -0.70710678);
          offsets[7] = vec2(-0.70710678, -0.70710678);

          for (int i = 0; i < 8; i++) {
            vec2 sampleUv = clamp(vUv + offsets[i] * texel, vec2(0.0), vec2(1.0));
            vec4 sampleDepth = texture2D(depthTexture, sampleUv);
            float depthValue = sampleDepth.a > 0.0
              ? max(sampleDepth.r, 1e-6)
              : cameraFar;
            float delta = max(0.0, log2(depthValue) - centerLogDepth);
            response += delta;
          }

          float shade = exp(-strength * response);
          gl_FragColor = vec4(baseColor.rgb * shade, baseColor.a);
        }
      `,
    });
  }

  public resize(width: number, height: number) {
    this.setSize(width, height);
    this.material.uniforms.resolution.value = new Vector2(width, height);
  }

  public setSettings(radius: number, strength: number) {
    this.material.uniforms.radius.value = radius;
    this.material.uniforms.strength.value = strength;
  }

  public renderPass(
    renderer: WebGLRenderer,
    camera: Camera,
    colorTexture: Texture,
    depthTexture: Texture,
  ) {
    this.material.uniforms.colorTexture.value = colorTexture;
    this.material.uniforms.depthTexture.value = depthTexture;
    this.material.uniforms.cameraFar.value = camera.far;

    this.renderToTarget(renderer);
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
