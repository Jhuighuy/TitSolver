/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { Matrix4 } from "three";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
 * Shared identity matrix used to initialize uniforms that will be overwritten
 * with the camera's inverse projection matrix at render time.
 */
export function matrixIdentity() {
  return new Matrix4();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export const fullscreenVertexShader = `
  // Passes operate on fullscreen quads/triangles and only need UVs.
  varying vec2 vUv;

  void main() {
    vUv = uv;
    gl_Position = projectionMatrix * modelViewMatrix * vec4(position, 1.0);
  }
`;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export const commonDepthShader = `
  // These uniforms are shared by all depth-aware passes.
  uniform sampler2D depthTexture;
  uniform vec2 resolution;
  uniform float cameraNear;
  uniform float cameraFar;
  uniform float isPerspective;

  float linearizeDepth(float depth) {
    // The stored depth texture is in clip-space depth. Convert it back to a
    // camera-space distance so kernels behave consistently across projection
    // modes and zoom levels.
    if (isPerspective > 0.5) {
      float zNdc = depth * 2.0 - 1.0;
      return (2.0 * cameraNear * cameraFar) /
        (cameraFar + cameraNear - zNdc * (cameraFar - cameraNear));
    }
    return cameraNear + depth * (cameraFar - cameraNear);
  }
`;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
