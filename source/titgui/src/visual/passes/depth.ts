/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { FloatType, RGBAFormat, type Texture, type WebGLRenderer } from "three";

import type { Camera } from "~/visual/camera";
import { FullscreenPass } from "~/visual/passes/fullscreen-pass";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
 * Converts an already-rendered raw depth texture into a reusable linear depth image.
 */
export class DepthPass extends FullscreenPass {
  public constructor() {
    super({
      format: RGBAFormat,
      type: FloatType,
      uniforms: {
        depthTexture: { value: null },
        cameraNear: { value: 0.01 },
        cameraFar: { value: 1000 },
        isPerspective: { value: 0 },
      },
      vertexShader: `
        varying vec2 vUv;

        void main() {
          vUv = uv;
          gl_Position = vec4(position.xy, 0.0, 1.0);
        }
      `,
      fragmentShader: `
        uniform sampler2D depthTexture;
        uniform float cameraNear;
        uniform float cameraFar;
        uniform int isPerspective;

        varying vec2 vUv;

        float linearizeDepth(float rawDepth) {
          if (isPerspective == 1) {
            float z = rawDepth * 2.0 - 1.0;
            return (2.0 * cameraNear * cameraFar) /
              max(cameraFar + cameraNear - z * (cameraFar - cameraNear), 1e-12);
          }
          return cameraNear + rawDepth * (cameraFar - cameraNear);
        }

        void main() {
          float rawDepth = texture2D(depthTexture, vUv).x;
          if (rawDepth >= 0.999999) {
            gl_FragColor = vec4(0.0, 0.0, 0.0, 0.0);
            return;
          }

          float linearDepth = linearizeDepth(rawDepth);
          gl_FragColor = vec4(linearDepth, linearDepth, linearDepth, 1.0);
        }
      `,
    });
  }

  public resize(width: number, height: number) {
    this.setSize(width, height);
  }

  public renderPass(
    renderer: WebGLRenderer,
    camera: Camera,
    depthTexture: Texture,
  ) {
    this.material.uniforms.depthTexture.value = depthTexture;
    this.material.uniforms.cameraNear.value = camera.near;
    this.material.uniforms.cameraFar.value = camera.far;
    this.material.uniforms.isPerspective.value =
      camera.projection === "perspective" ? 1 : 0;

    this.renderToTarget(renderer);
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
