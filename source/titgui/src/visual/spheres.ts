/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import {
  BufferGeometry,
  Float32BufferAttribute,
  Points,
  ShaderMaterial,
  Texture,
  Vector2,
} from "three";

import { assert } from "~/utils";
import {
  type ColorMap,
  type ColorRange,
  colorMaps,
  colorMapToTexture,
} from "~/visual/color-map";
import type { Field } from "~/visual/fields";
import type { Particles } from "~/visual/particles";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export class Spheres
  extends Points<BufferGeometry, ShaderMaterial>
  implements Particles
{
  public constructor() {
    super();
    this.frustumCulled = false;
    this.geometry = new BufferGeometry();
    this.material = new ShaderMaterial({
      depthTest: true,
      vertexShader: vertexShaderSource,
      fragmentShader: fragmentShaderSource,
      uniforms: {
        minValue: { value: 0 },
        maxValue: { value: 1 },
        colorMap: { value: colorMapToTexture(colorMaps.jet) },
        nanColor: { value: colorMaps.jet.nanColor },
        pointSize: { value: 25 },
        viewport: { value: new Vector2(1, 1) },
      },
    });
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  public dispose() {
    this.geometry.dispose();
    this.material.dispose();
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  public setData(
    field: Field,
    colorValues: Float32Array,
    positionValues: Float32Array,
  ) {
    assert(field.count === colorValues.length);
    assert(field.count === positionValues.length / 3);
    this.geometry.setAttribute(
      "value",
      new Float32BufferAttribute(colorValues, 1),
    );
    this.geometry.setAttribute(
      "position",
      new Float32BufferAttribute(positionValues, 3),
    );
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  public setPointSize(size: number) {
    assert(Number.isFinite(size) && size > 0);
    this.material.uniforms.pointSize.value = size;
  }

  public setViewport(width: number, height: number) {
    this.material.uniforms.viewport.value = new Vector2(width, height);
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  public setColorRange({ min, max }: ColorRange) {
    assert(Number.isFinite(min));
    assert(Number.isFinite(max) && max >= min);
    this.material.uniforms.minValue.value = min;
    this.material.uniforms.maxValue.value = max;
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  public setColorMap(colorMap: ColorMap) {
    if (this.material.uniforms.colorMap.value) {
      assert(this.material.uniforms.colorMap.value instanceof Texture);
      this.material.uniforms.colorMap.value.dispose();
    }
    this.material.uniforms.colorMap.value = colorMapToTexture(colorMap);
    this.material.uniforms.nanColor.value = colorMap.nanColor;
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

const vertexShaderSource = `
  uniform float pointSize;

  attribute float value;
  varying float vValue;
  varying float vPointSize;
  varying vec4 vMvCenter;

  void main() {
    vValue = value;

    vMvCenter = modelViewMatrix * vec4(position, 1.0);
    vPointSize = -pointSize / vMvCenter.z;
    gl_PointSize = vPointSize;
    gl_Position = projectionMatrix * vMvCenter;
  }
`;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

const fragmentShaderSource = `
  const float epsilon = 1e-12;

  uniform sampler2D colorMap;
  uniform vec3 nanColor;
  uniform float minValue;
  uniform float maxValue;
  uniform vec2 viewport;
  uniform mat4 projectionMatrix;

  varying float vValue;
  varying float vPointSize;
  varying vec4 vMvCenter;

  void main() {
    vec2 unit = 2.0 * gl_PointCoord - vec2(1.0);
    float r2 = dot(unit, unit);
    if (r2 > 1.0) discard;

    float viewRadiusX = abs(vMvCenter.z) * vPointSize /
      max(viewport.x * projectionMatrix[0][0], epsilon);
    float viewRadiusY = abs(vMvCenter.z) * vPointSize /
      max(viewport.y * projectionMatrix[1][1], epsilon);
    float sphereRadius = 0.5 * (viewRadiusX + viewRadiusY);
    vec3 sphereOffset = vec3(
      unit * sphereRadius,
      sqrt(max(1.0 - r2, 0.0)) * sphereRadius
    );
    vec4 clipPosition = projectionMatrix * (vMvCenter + vec4(sphereOffset, 0.0));
    gl_FragDepth = 0.5 * (clipPosition.z / clipPosition.w) + 0.5;

    if (vValue != vValue) {
      gl_FragColor = vec4(nanColor, 1.0);
    } else {
      float denom = max(maxValue - minValue, epsilon);
      float scaled = clamp((vValue - minValue) / denom, 0.0, 1.0);
      gl_FragColor = texture2D(colorMap, vec2(scaled, 0.5));
    }
  }
`;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
