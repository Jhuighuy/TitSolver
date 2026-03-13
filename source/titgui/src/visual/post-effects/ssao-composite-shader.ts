/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { fullscreenVertexShader } from "~/visual/post-effects/common";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
 * Multiplies the blurred SSAO factor back into the untouched scene color.
 *
 * `tDiffuse` contains the blurred AO term from the previous pass.
 * `colorTexture` contains the original scene color from the dedicated render
 * target so the AO pass chain does not accumulate its own darkening artifacts.
 */
export const ssaoCompositeShader = {
  name: "TitSsaoCompositePass",
  uniforms: {
    tDiffuse: { value: null },
    colorTexture: { value: null },
  },
  vertexShader: fullscreenVertexShader,
  fragmentShader: `
    uniform sampler2D tDiffuse;
    uniform sampler2D colorTexture;

    varying vec2 vUv;

    void main() {
      vec4 color = texture2D(colorTexture, vUv);
      float ao = texture2D(tDiffuse, vUv).r;
      // AO is a scalar lighting attenuation term.
      gl_FragColor = vec4(color.rgb * ao, color.a);
    }
  `,
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
