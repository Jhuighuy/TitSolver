/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import {
  BufferGeometry,
  Color,
  DataTexture,
  Float32BufferAttribute,
  LinearFilter,
  Points,
  RGBAFormat,
  type Scene,
  ShaderMaterial,
  type Texture,
  Vector3,
} from "three";

import { assert } from "~/utils";

import { type ColorMapType, colorMaps } from "~/visual/ColorMap";

import fragmentShaderSource from "./shaders/particle-fragment.glsl?raw";
import vertexShaderSource from "./shaders/particle-vertex.glsl?raw";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export type FieldMap = Record<string, ArrayLike<number>>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

interface FrameOptions {
  colorMap?: Texture;
  colorField?: string;
  pointSize?: number;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export class Frame {
  readonly object3D: Points;
  readonly particleCount: number;

  public min: number;
  public max: number;

  private geometry: BufferGeometry;
  private material: ShaderMaterial;

  constructor(data: FieldMap, options: FrameOptions = {}) {
    const { r, rho } = data;
    assert(r && rho, "Data must contain 'r' and 'rho' fields.");

    assert(r.length % rho.length === 0);
    const dim = r.length / rho.length;
    assert(dim === 2 || dim === 3, `Invalid dimensionality: ${dim}.`);

    this.particleCount = rho.length;

    // --- Setup geometry ---
    this.geometry = new BufferGeometry();

    this.geometry.setAttribute(
      "position",
      new Float32BufferAttribute(dim === 3 ? r : expand2Dto3D(r), 3)
    );

    const colorField = options.colorField || "rho";
    const scalar = data[colorField];
    if (!scalar) throw new Error(`Color field "${colorField}" not found`);

    this.geometry.setAttribute("scalar", new Float32BufferAttribute(scalar, 1));

    // --- Compute min/max of scalar field for normalization ---
    let min = Number.POSITIVE_INFINITY;
    let max = Number.NEGATIVE_INFINITY;
    for (let i = 0; i < scalar.length; ++i) {
      const val = scalar[i];
      if (val < min) min = val;
      if (val > max) max = val;
    }
    this.min = min;
    this.max = max;

    // --- Material with scalar color mapping ---
    this.material = new ShaderMaterial({
      vertexShader: vertexShaderSource,
      fragmentShader: fragmentShaderSource,
      uniforms: {
        minValue: { value: min },
        maxValue: { value: max },
        colorMap: { value: null },
        pointSize: { value: options.pointSize ?? 0.1 },
        lightPosition: { value: new Vector3(10, 10, -10) },
        ambientLightColor: { value: new Color(0xaaaaaa) },
        pointLightColor: { value: new Color(0xffffff) },
      },
      transparent: true,
      depthTest: true,
    });
    this.setColorMap("jet");

    this.object3D = new Points(this.geometry, this.material);
  }

  addTo(scene: Scene) {
    scene.add(this.object3D);
  }

  removeFrom(scene: Scene) {
    scene.remove(this.object3D);
  }

  setColorMap(colorMap: ColorMapType) {
    this.material.uniforms.colorMap.value = createColorMapTextureFromRGBPoints(
      colorMaps[colorMap].rgbPoints.flat()
    );
  }

  setParticleSize(size: number) {
    this.material.uniforms.pointSize.value = size / 100.0;
  }

  setMinMax(min: number, max: number) {
    this.min = min;
    this.max = max;
    this.material.uniforms.minValue.value = min;
    this.material.uniforms.maxValue.value = max;
  }

  dispose() {
    this.geometry.dispose();
    this.material.dispose();
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

function expand2Dto3D(input: ArrayLike<number>): Float32Array {
  const n = input.length / 2;
  const output = new Float32Array(n * 3);
  for (let i = 0; i < n; i++) {
    output[3 * i + 0] = input[2 * i + 0]; // x
    output[3 * i + 1] = input[2 * i + 1]; // y
    output[3 * i + 2] = 0; // z
  }
  return output;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export function createColorMapTextureFromRGBPoints(
  RGBPoints: number[],
  resolution = 256
): DataTexture {
  if (RGBPoints.length % 4 !== 0) {
    throw new Error(
      "RGBPoints must be a flat array of [value, r, g, b] tuples"
    );
  }

  const colorStops: { value: number; color: [number, number, number] }[] = [];
  for (let i = 0; i < RGBPoints.length; i += 4) {
    colorStops.push({
      value: RGBPoints[i],
      color: [RGBPoints[i + 1], RGBPoints[i + 2], RGBPoints[i + 3]],
    });
  }

  const minVal = colorStops[0].value;
  const maxVal = colorStops[colorStops.length - 1].value;

  const data = new Uint8Array(resolution * 4); // RGBA

  for (let i = 0; i < resolution; i++) {
    const t = i / (resolution - 1);
    const scalar = minVal + t * (maxVal - minVal);

    // Find the interval [lower, upper]
    let lower = colorStops[0];
    let upper = colorStops[colorStops.length - 1];

    for (let j = 0; j < colorStops.length - 1; j++) {
      if (scalar >= colorStops[j].value && scalar <= colorStops[j + 1].value) {
        lower = colorStops[j];
        upper = colorStops[j + 1];
        break;
      }
    }

    const dt = (scalar - lower.value) / (upper.value - lower.value);
    const color: [number, number, number] = [
      lower.color[0] + dt * (upper.color[0] - lower.color[0]),
      lower.color[1] + dt * (upper.color[1] - lower.color[1]),
      lower.color[2] + dt * (upper.color[2] - lower.color[2]),
    ];

    data[i * 4 + 0] = Math.round(color[0] * 255);
    data[i * 4 + 1] = Math.round(color[1] * 255);
    data[i * 4 + 2] = Math.round(color[2] * 255);
    data[i * 4 + 3] = 255; // Alpha
  }

  const texture = new DataTexture(data, resolution, 1, RGBAFormat);
  texture.needsUpdate = true;
  texture.minFilter = LinearFilter;
  texture.magFilter = LinearFilter;
  texture.generateMipmaps = false;

  return texture;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
