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

  void main() {
    vValue = value;

    vec4 mvPosition = modelViewMatrix * vec4(position, 1.0);
    gl_PointSize = -pointSize / mvPosition.z;
    gl_Position = projectionMatrix * mvPosition;
  }
`;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

const fragmentShaderSource = `
  const float epsilon = 1e-12;

  uniform sampler2D colorMap;
  uniform vec3 nanColor;
  uniform float minValue;
  uniform float maxValue;

  varying float vValue;

  void main() {
    float r = length(gl_PointCoord - vec2(0.5));
    if (r > 0.5) discard;

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
