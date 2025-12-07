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

import { assert, expandTo3D } from "~/utils";
import { type ColorMapName, createColorMapTexture } from "~/visual/color-map";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export type FieldMap = Record<
  string,
  {
    min: number;
    max: number;
    data: ArrayLike<number>;
  }
>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export class Particles extends Points<BufferGeometry, ShaderMaterial> {
  public constructor() {
    super();
    this.geometry = new BufferGeometry();
    this.material = new ShaderMaterial({
      depthTest: true,
      vertexShader: vertexShaderSource,
      fragmentShader: fragmentShaderSource,
      uniforms: {
        minValue: { value: 0 },
        maxValue: { value: 1 },
        colorMap: { value: createColorMapTexture("jet") },
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

  public setData(data: FieldMap, colorField: string) {
    // Note: We recreate the geometry here to avoid visual glitches.
    this.geometry.dispose();
    this.geometry = new BufferGeometry();

    const { r, rho } = data;
    assert(r && rho, "Data must contain 'r' and 'rho' fields.");
    assert(rho.data.length > 0);
    assert(r.data.length % rho.data.length === 0);
    const dim = r.data.length / rho.data.length;
    assert(dim === 2 || dim === 3, `Invalid dimensionality: ${dim}.`);
    this.geometry.setAttribute(
      "position",
      new Float32BufferAttribute(dim === 3 ? r.data : expandTo3D(r.data), 3)
    );

    /** @todo Support for vector fields. */
    const scalar = data[colorField];
    assert(scalar, `Color field "${colorField}" not found`);
    assert(scalar.data.length === rho.data.length);
    this.geometry.setAttribute(
      "scalar",
      new Float32BufferAttribute(scalar.data, 1)
    );
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  public setColorRange(min: number, max: number) {
    this.material.uniforms.minValue.value = min;
    this.material.uniforms.maxValue.value = max;
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  public setColorMap(colorMap: ColorMapName) {
    if (this.material.uniforms.colorMap.value) {
      assert(this.material.uniforms.colorMap.value instanceof Texture);
      this.material.uniforms.colorMap.value.dispose();
    }
    this.material.uniforms.colorMap.value = createColorMapTexture(colorMap);
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

const vertexShaderSource = `
  uniform float pointSize;

  attribute float scalar;
  varying float vScalar;

  void main() {
    vScalar = scalar;

    vec4 mvPosition = modelViewMatrix * vec4(position, 1.0);
    gl_PointSize = -pointSize / mvPosition.z;
    gl_Position = projectionMatrix * mvPosition;
  }
`;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

const fragmentShaderSource = `
  uniform sampler2D colorMap;
  uniform float minValue;
  uniform float maxValue;

  varying float vScalar;

  void main() {
    float t = clamp((vScalar - minValue) / (maxValue - minValue), 0.0, 1.0);
    vec4 color = texture2D(colorMap, vec2(t, 0.5));

    float r = length(gl_PointCoord - vec2(0.5));
    if (r > 0.5) discard;

    gl_FragColor = color;
  }
`;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
