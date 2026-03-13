/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { Vector2 } from "three";

import {
  commonDepthShader,
  fullscreenVertexShader,
} from "~/visual/post-effects/common";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
 * Eye-Dome Lighting pass.
 *
 * EDL is applied after SSAO composition. It darkens local depth
 * discontinuities and silhouettes to improve shape readability in point-heavy
 * views, while leaving the underlying color mapping intact.
 */
export const edlShader = {
  name: "TitEdlPass",
  uniforms: {
    tDiffuse: { value: null },
    depthTexture: { value: null },
    resolution: { value: new Vector2(1, 1) },
    cameraNear: { value: 0.01 },
    cameraFar: { value: 1000 },
    isPerspective: { value: 0 },
    radius: { value: 1.5 },
    strength: { value: 1.2 },
  },
  vertexShader: fullscreenVertexShader,
  fragmentShader: `
    uniform sampler2D tDiffuse;
    uniform float radius;
    uniform float strength;

    varying vec2 vUv;

    ${commonDepthShader}

    void main() {
      vec4 baseColor = texture2D(tDiffuse, vUv);
      // Ignore transparent/background pixels.
      if (baseColor.a <= 0.0) {
        gl_FragColor = baseColor;
        return;
      }

      float centerRawDepth = texture2D(depthTexture, vUv).x;
      if (centerRawDepth >= 0.999999) {
        gl_FragColor = baseColor;
        return;
      }

      float centerDepth = max(linearizeDepth(centerRawDepth), 1e-6);
      float centerLogDepth = log2(centerDepth);
      // Radius is measured in pixels of the output image.
      vec2 texel = radius / resolution;
      float response = 0.0;
      float samples = 0.0;

      for (int y = -1; y <= 1; y++) {
        for (int x = -1; x <= 1; x++) {
          if (x == 0 && y == 0) continue;

          vec2 sampleUv = clamp(
            vUv + vec2(float(x), float(y)) * texel,
            vec2(0.0),
            vec2(1.0)
          );
          float rawDepth = texture2D(depthTexture, sampleUv).x;
          float sampleDepth = rawDepth >= 0.999999
            ? cameraFar
            : max(linearizeDepth(rawDepth), 1e-6);
          response += max(0.0, log2(sampleDepth) - centerLogDepth);
          samples += 1.0;
        }
      }

      if (samples <= 0.0) {
        gl_FragColor = baseColor;
        return;
      }

      float responseAvg = response / samples;
      // Strength scales the perceived contour darkening.
      float shade = 1.0 / (1.0 + strength * 2.0 * responseAvg);
      gl_FragColor = vec4(baseColor.rgb * shade, baseColor.a);
    }
  `,
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
