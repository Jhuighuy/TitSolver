/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import {
  BufferAttribute,
  InstancedBufferAttribute,
  InstancedBufferGeometry,
  Mesh,
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

export type GlyphScaleMode = "magnitude" | "uniform";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export class Glyphs
  extends Mesh<InstancedBufferGeometry, ShaderMaterial>
  implements Particles
{
  public constructor() {
    super();
    this.frustumCulled = false;
    this.geometry = new InstancedBufferGeometry();
    this.geometry.setAttribute(
      "position",
      new BufferAttribute(glyphVertices, 3),
    );
    this.material = new ShaderMaterial({
      depthTest: true,
      vertexShader: vertexShaderSource,
      fragmentShader: fragmentShaderSource,
      uniforms: {
        minValue: { value: 0 },
        maxValue: { value: 1 },
        colorMap: { value: colorMapToTexture(colorMaps.jet) },
        nanColor: { value: colorMaps.jet.nanColor },
        selectedColor: { value: [0.99, 0.76, 0.18] },
        lengthScale: { value: 0.02 },
        widthScale: { value: 0.02 * glyphWidthRatio },
        scaleByMagnitude: { value: 1.0 },
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

    const fieldValues = new Float32Array(field.count * 3);
    for (let i = 0; i < field.count; i++) {
      const components = field.components(i);
      fieldValues[i * 3 + 0] = components[0];
      fieldValues[i * 3 + 1] = components[1];
      fieldValues[i * 3 + 2] = components[2] ?? 0;
    }

    this.geometry.instanceCount = field.count;
    this.geometry.setAttribute(
      "value",
      new InstancedBufferAttribute(colorValues, 1),
    );
    this.geometry.setAttribute(
      "vector",
      new InstancedBufferAttribute(fieldValues, 3),
    );
    this.geometry.setAttribute(
      "center",
      new InstancedBufferAttribute(positionValues, 3),
    );
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  public setSelection(selectionValues: Float32Array) {
    this.geometry.setAttribute(
      "selection",
      new InstancedBufferAttribute(selectionValues, 1),
    );
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  public setScale(scale: number) {
    assert(Number.isFinite(scale) && scale > 0);
    this.material.uniforms.lengthScale.value = scale;
    this.material.uniforms.widthScale.value = scale * glyphWidthRatio;
  }

  public setScaleMode(mode: GlyphScaleMode) {
    this.material.uniforms.scaleByMagnitude.value =
      mode === "magnitude" ? 1 : 0;
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

const glyphWidthRatio = 0.4;

const glyphVertices = new Float32Array(
  [
    [0, -0.18, 0],
    [0.75, -0.18, 0],
    [0.75, 0.18, 0],
    [0, -0.18, 0],
    [0.75, 0.18, 0],
    [0, 0.18, 0],
    [0.6, -0.35, 0],
    [1.0, 0, 0],
    [0.6, 0.35, 0],
  ].flat(),
);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

const vertexShaderSource = `
  const float epsilon = 1e-12;

  attribute float value;
  attribute float selection;
  attribute vec3 vector;
  attribute vec3 center;

  uniform float lengthScale;
  uniform float widthScale;
  uniform float scaleByMagnitude;

  varying float vValue;
  varying float vSelection;

  void main() {
    vValue = value;
    vSelection = selection;

    float len = length(vector);

    vec2 dir;
    float lenXY = length(vector.xy);
    if (lenXY <= epsilon) {
      dir = vec2(1.0, 0.0);
    } else {
      dir = vector.xy / lenXY;
    }
    vec2 ortho = vec2(-dir.y, dir.x);

    float scaleFactor = mix(1.0, len, scaleByMagnitude);
    float scaledLen = scaleFactor * lengthScale;
    float scaledWidth = scaleFactor * widthScale;
    vec2 offset = dir * (position.x * scaledLen) + ortho * (position.y * scaledWidth);

    vec3 worldPosition = vec3(center.xy + offset, center.z);
    vec4 mvPosition = modelViewMatrix * vec4(worldPosition, 1.0);
    gl_Position = projectionMatrix * mvPosition;
  }
`;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

const fragmentShaderSource = `
  const float epsilon = 1e-12;

  uniform sampler2D colorMap;
  uniform vec3 nanColor;
  uniform vec3 selectedColor;
  uniform float minValue;
  uniform float maxValue;

  varying float vValue;
  varying float vSelection;

  void main() {
    vec3 color;
    if (vValue != vValue) {
      color = nanColor;
    } else {
      float denom = max(maxValue - minValue, epsilon);
      float scaled = clamp((vValue - minValue) / denom, 0.0, 1.0);
      color = texture2D(colorMap, vec2(scaled, 0.5)).rgb;
    }

    if (vSelection > 0.5) {
      color = mix(color, selectedColor, 0.5);
    }

    gl_FragColor = vec4(color, 1.0);
  }
`;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
