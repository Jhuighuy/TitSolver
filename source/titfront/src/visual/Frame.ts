/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import {
  BufferGeometry,
  Color,
  Float32BufferAttribute,
  Points,
  type Scene,
  ShaderMaterial,
  type Texture,
  Vector3,
} from "three";

import { assert } from "~/utils";

import { type ColorMapName, createColorMapTexture } from "~/visual/ColorMap";

import fragmentShaderSource from "./shaders/particle-fragment.glsl?raw";
import vertexShaderSource from "./shaders/particle-vertex.glsl?raw";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export type Field = {
  min: number;
  max: number;
  data: ArrayLike<number>;
};

export type FieldMap = Record<string, Field>;

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

  private geometry: BufferGeometry;
  private material: ShaderMaterial;

  constructor(data: FieldMap, options: FrameOptions = {}) {
    const { r, rho } = data;
    assert(r && rho, "Data must contain 'r' and 'rho' fields.");

    assert(r.data.length % rho.data.length === 0);
    const dim = r.data.length / rho.data.length;
    assert(dim === 2 || dim === 3, `Invalid dimensionality: ${dim}.`);

    this.particleCount = rho.data.length;

    // --- Setup geometry ---
    this.geometry = new BufferGeometry();

    this.geometry.setAttribute(
      "position",
      new Float32BufferAttribute(dim === 3 ? r.data : expand2Dto3D(r.data), 3)
    );

    const colorField = options.colorField || "rho";
    const scalar = data[colorField];
    if (!scalar) throw new Error(`Color field "${colorField}" not found`);

    this.geometry.setAttribute(
      "scalar",
      new Float32BufferAttribute(scalar.data, 1)
    );

    // --- Material with scalar color mapping ---
    this.material = new ShaderMaterial({
      vertexShader: vertexShaderSource,
      fragmentShader: fragmentShaderSource,
      uniforms: {
        minValue: { value: scalar.min },
        maxValue: { value: scalar.max },
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

  setColorMap(colorMap: ColorMapName) {
    this.material.uniforms.colorMap.value = createColorMapTexture(colorMap);
  }

  setColorRange(min: number, max: number) {
    this.material.uniforms.minValue.value = min;
    this.material.uniforms.maxValue.value = max;
  }

  setParticleSize(size: number) {
    this.material.uniforms.pointSize.value = size / 100.0;
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
