/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { Vector2 } from "three";

import {
  commonDepthShader,
  fullscreenVertexShader,
  matrixIdentity,
} from "~/visual/post-effects/common";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
 * Produces a monochrome ambient-occlusion factor from scene depth.
 *
 * Output:
 * - white: unoccluded
 * - dark: occluded
 *
 * This pass does not touch the input color; it only generates the AO term
 * that later passes blur and composite.
 */
export const ssaoFactorShader = {
  name: "TitSsaoFactorPass",
  uniforms: {
    tDiffuse: { value: null },
    depthTexture: { value: null },
    resolution: { value: new Vector2(1, 1) },
    inverseProjectionMatrix: { value: matrixIdentity() },
    cameraNear: { value: 0.01 },
    cameraFar: { value: 1000 },
    isPerspective: { value: 0 },
    radius: { value: 4.0 },
    intensity: { value: 1.5 },
    bias: { value: 0.01 },
  },
  vertexShader: fullscreenVertexShader,
  fragmentShader: `
    uniform mat4 inverseProjectionMatrix;
    uniform float radius;
    uniform float intensity;
    uniform float bias;

    varying vec2 vUv;

    ${commonDepthShader}

    vec3 reconstructViewPosition(vec2 uv, float depth) {
      // Reconstruct a view-space point from screen UV + depth so we can
      // reason about local geometry in camera space.
      vec4 clip = vec4(uv * 2.0 - vec2(1.0), depth * 2.0 - 1.0, 1.0);
      vec4 view = inverseProjectionMatrix * clip;
      return view.xyz / max(view.w, 1e-6);
    }

    vec3 reconstructNormal(vec2 uv, float centerRawDepth) {
      // Approximate the local normal from depth derivatives. This avoids
      // introducing a separate normal render target for now.
      vec2 texel = 1.0 / resolution;
      vec3 center = reconstructViewPosition(uv, centerRawDepth);

      float depthRight = texture2D(depthTexture, clamp(uv + vec2(texel.x, 0.0), vec2(0.0), vec2(1.0))).x;
      float depthLeft = texture2D(depthTexture, clamp(uv - vec2(texel.x, 0.0), vec2(0.0), vec2(1.0))).x;
      float depthUp = texture2D(depthTexture, clamp(uv + vec2(0.0, texel.y), vec2(0.0), vec2(1.0))).x;
      float depthDown = texture2D(depthTexture, clamp(uv - vec2(0.0, texel.y), vec2(0.0), vec2(1.0))).x;

      vec3 right = reconstructViewPosition(uv + vec2(texel.x, 0.0), depthRight >= 0.999999 ? centerRawDepth : depthRight);
      vec3 left = reconstructViewPosition(uv - vec2(texel.x, 0.0), depthLeft >= 0.999999 ? centerRawDepth : depthLeft);
      vec3 up = reconstructViewPosition(uv + vec2(0.0, texel.y), depthUp >= 0.999999 ? centerRawDepth : depthUp);
      vec3 down = reconstructViewPosition(uv - vec2(0.0, texel.y), depthDown >= 0.999999 ? centerRawDepth : depthDown);

      vec3 dx = length(right - center) < length(center - left)
        ? right - center
        : center - left;
      vec3 dy = length(up - center) < length(center - down)
        ? up - center
        : center - down;
      vec3 normal = normalize(cross(dx, dy));
      vec3 viewDirection = normalize(-center);
      // Keep normals facing the camera so the AO horizon test is stable.
      return dot(normal, viewDirection) >= 0.0 ? normal : -normal;
    }

    void main() {
      // Background pixels stay fully lit.
      float centerRawDepth = texture2D(depthTexture, vUv).x;
      if (centerRawDepth >= 0.999999) {
        gl_FragColor = vec4(1.0);
        return;
      }

      float centerDepth = max(linearizeDepth(centerRawDepth), 1e-6);
      vec3 centerPosition = reconstructViewPosition(vUv, centerRawDepth);
      vec3 centerNormal = reconstructNormal(vUv, centerRawDepth);
      // Radius is expressed in pixels of the target image.
      vec2 texel = radius / resolution;
      float occlusion = 0.0;
      float samples = 0.0;

      for (int y = -2; y <= 2; y++) {
        for (int x = -2; x <= 2; x++) {
          if (x == 0 && y == 0) continue;

          vec2 dir = vec2(float(x), float(y));
          float len2 = max(dot(dir, dir), 1.0);
          vec2 sampleUv = clamp(vUv + dir * texel, vec2(0.0), vec2(1.0));
          float rawDepth = texture2D(depthTexture, sampleUv).x;
          if (rawDepth >= 0.999999) continue;

          vec3 samplePosition = reconstructViewPosition(sampleUv, rawDepth);
          vec3 deltaPosition = samplePosition - centerPosition;
          // The occlusion term prefers nearby geometry that rises above the
          // tangent plane defined by the center normal.
          float distanceSq = max(dot(deltaPosition, deltaPosition), 1e-6);
          float distance = sqrt(distanceSq);
          float normalHeight = dot(centerNormal, deltaPosition);
          float proximity = max(1.0 - distance / max(centerDepth, 1e-3), 0.0);
          float horizon = max(normalHeight - bias, 0.0);
          float weight = 1.0 / len2;
          occlusion += weight * horizon * proximity / (0.1 + distance);
          samples += weight;
        }
      }

      if (samples <= 0.0) {
        gl_FragColor = vec4(1.0);
        return;
      }

      float occlusionAvg = occlusion / samples;
      // Convert "amount of blocking" into a lighting factor.
      float ao = clamp(1.0 - intensity * 24.0 * occlusionAvg, 0.0, 1.0);
      gl_FragColor = vec4(vec3(ao), 1.0);
    }
  `,
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
