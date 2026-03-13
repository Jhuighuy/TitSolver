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
 * Depth-aware separable blur for the SSAO factor.
 *
 * The blur is run twice, first with `direction = (1, 0)` and then with
 * `direction = (0, 1)`. Depth similarity attenuates the blur across hard
 * silhouettes so AO stays attached to local geometry.
 */
export const ssaoBlurShader = {
  name: "TitSsaoBlurPass",
  uniforms: {
    tDiffuse: { value: null },
    depthTexture: { value: null },
    resolution: { value: new Vector2(1, 1) },
    cameraNear: { value: 0.01 },
    cameraFar: { value: 1000 },
    isPerspective: { value: 0 },
    direction: { value: new Vector2(1, 0) },
  },
  vertexShader: fullscreenVertexShader,
  fragmentShader: `
    uniform sampler2D tDiffuse;
    uniform vec2 direction;

    varying vec2 vUv;

    ${commonDepthShader}

    float sampleAo(vec2 uv) {
      return texture2D(tDiffuse, uv).r;
    }

    void main() {
      // Use scene depth, not AO itself, to decide whether neighboring samples
      // should be mixed into the blur result.
      float centerRawDepth = texture2D(depthTexture, vUv).x;
      float centerDepth = centerRawDepth >= 0.999999
        ? cameraFar
        : linearizeDepth(centerRawDepth);

      vec2 texel = direction / resolution;

      float sum = sampleAo(vUv) * 0.227027;
      float weightSum = 0.227027;

      for (int i = 1; i <= 2; i++) {
        float offset = float(i);
        vec2 delta = texel * offset;
        float gaussian = i == 1 ? 0.316216 : 0.070270;

        vec2 uvPos = clamp(vUv + delta, vec2(0.0), vec2(1.0));
        vec2 uvNeg = clamp(vUv - delta, vec2(0.0), vec2(1.0));

        float depthPosRaw = texture2D(depthTexture, uvPos).x;
        float depthNegRaw = texture2D(depthTexture, uvNeg).x;
        float depthPos = depthPosRaw >= 0.999999 ? cameraFar : linearizeDepth(depthPosRaw);
        float depthNeg = depthNegRaw >= 0.999999 ? cameraFar : linearizeDepth(depthNegRaw);

        // Penalize mixing samples that lie on a very different depth layer.
        float bilateralPos = exp(-abs(depthPos - centerDepth) * 8.0 / max(centerDepth, 1e-3));
        float bilateralNeg = exp(-abs(depthNeg - centerDepth) * 8.0 / max(centerDepth, 1e-3));

        float weightPos = gaussian * bilateralPos;
        float weightNeg = gaussian * bilateralNeg;

        sum += sampleAo(uvPos) * weightPos;
        sum += sampleAo(uvNeg) * weightNeg;
        weightSum += weightPos + weightNeg;
      }

      gl_FragColor = vec4(vec3(sum / max(weightSum, 1e-6)), 1.0);
    }
  `,
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
