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

import {
  type ColorMap,
  type ColorRange,
  colorMaps,
  colorMapToTexture,
} from "~/renderer-common/visual/color-map";
import type { Field } from "~/renderer-common/visual/fields";
import type {
  Particles,
  ShadingMode,
} from "~/renderer-common/visual/particles";
import { assert } from "~/shared/utils";

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
      alphaToCoverage: true,
      depthTest: true,
      depthWrite: true,
      vertexShader: vertexShaderSource,
      fragmentShader: fragmentShaderSource,
      uniforms: {
        minValue: { value: 0 },
        maxValue: { value: 1 },
        colorMap: { value: colorMapToTexture(colorMaps.jet) },
        nanColor: { value: colorMaps.jet.nanColor },
        pointSize: { value: 25 },
        viewportHeight: { value: 1 },
        shadingMix: { value: 0 },
      },
    });
    (this.material.extensions as { fragDepth?: boolean }).fragDepth = true;
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

  public setShadingMode(mode: ShadingMode) {
    this.material.uniforms.shadingMix.value = mode === "shaded" ? 1 : 0;
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  public setViewportSize(width: number, height: number) {
    assert(Number.isFinite(width) && width > 0);
    assert(Number.isFinite(height) && height > 0);
    this.material.uniforms.viewportHeight.value = height;
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
  uniform float viewportHeight;

  attribute float value;

  varying float vValue;
  varying float vViewRadius;
  varying vec3 vCenterView;

  void main() {
    vValue = value;

    vec4 mvPosition = modelViewMatrix * vec4(position, 1.0);
    vCenterView = mvPosition.xyz;

    float perspectiveFactor = step(0.5, abs(projectionMatrix[2][3]));
    vViewRadius =
      (pointSize / (viewportHeight * projectionMatrix[1][1])) *
      mix(1.0, -mvPosition.z, perspectiveFactor);

    gl_PointSize = pointSize;
    gl_Position = projectionMatrix * mvPosition;
  }
`;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

const fragmentShaderSource = `
  const float epsilon = 1.0e-12;

  uniform mat4 projectionMatrix;
  uniform float pointSize;
  uniform sampler2D colorMap;
  uniform vec3 nanColor;
  uniform float minValue;
  uniform float maxValue;
  uniform float shadingMix;

  varying float vValue;
  varying vec3 vCenterView;
  varying float vViewRadius;

  void main() {
    vec2 xy = 2.0 * (gl_PointCoord - vec2(0.5));
    float radiusSquared = dot(xy, xy);
    if (radiusSquared > 1.0) discard;

    vec3 normal = vec3(xy.x, -xy.y, sqrt(max(1.0 - radiusSquared, 0.0)));
    vec3 surfaceView = vCenterView + normal * vViewRadius;
    vec4 clipSurface = projectionMatrix * vec4(surfaceView, 1.0);
    gl_FragDepth = clamp(0.5 * (clipSurface.z / clipSurface.w) + 0.5, 0.0, 1.0);

    if (vValue != vValue) {
      gl_FragColor = vec4(nanColor, 1.0);
    } else {
      float denom = max(maxValue - minValue, epsilon);
      float scaled = clamp((vValue - minValue) / denom, 0.0, 1.0);
      vec3 baseColor = texture2D(colorMap, vec2(scaled, 0.5)).rgb;

      vec3 lightDir = normalize(vec3(-0.4, 0.6, 0.7));
      float diffuse = 0.35 + 0.65 * max(dot(normal, lightDir), 0.0);
      float specular = pow(max(dot(reflect(-lightDir, normal), vec3(0.0, 0.0, 1.0)), 0.0), 16.0);
      vec3 shadedColor = clamp(baseColor * diffuse + vec3(0.15) * specular, 0.0, 1.0);

      gl_FragColor = vec4(mix(baseColor, shadedColor, shadingMix), 1.0);
    }
  }
`;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
