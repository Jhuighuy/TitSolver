/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { Vector3 } from "three";
import { create } from "zustand";

import type { BackgroundColorName } from "~/visual/background-color";
import type { Projection } from "~/visual/camera";
import type { ColorMapName } from "~/visual/color-map";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export type ColorRangeType = "auto" | "custom";

type FieldsState = {
  field: string;
  colorMapName: ColorMapName;
  colorRangeType: ColorRangeType;
  colorRangeMin: number;
  colorRangeMax: number;
  setField: (field: string) => void;
  setColorMapName: (colorMap: ColorMapName) => void;
  setColorRangeType: (colorRangeType: ColorRangeType) => void;
  setColorRange: (min: number, max: number) => void;
};

export const useFieldsStore = create<FieldsState>((set) => ({
  field: "rho",
  colorMapName: "turbo",
  colorRangeType: "auto",
  colorRangeMin: 0,
  colorRangeMax: 1,
  setField: (field) => set({ field }),
  setColorMapName: (colorMapName) => set({ colorMapName }),
  setColorRangeType: (colorRangeType) => set({ colorRangeType }),
  setColorRange: (min, max) => set({ colorRangeMin: min, colorRangeMax: max }),
}));

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export type ColorLegendLocation = "left" | "bottom" | "none";

type ColorLegendState = {
  legendLocation: ColorLegendLocation;
  legendNumTicks: number;
  title: string | null;
  setLegendLocation: (legendLocation: ColorLegendLocation) => void;
  setLegendNumTicks: (legendNumTicks: number) => void;
  setTitle: (title: string | null) => void;
};

export const useColorLegendStore = create<ColorLegendState>((set) => ({
  legendLocation: "bottom",
  legendNumTicks: 5,
  title: null,
  setLegendLocation: (legendLocation) => set({ legendLocation }),
  setLegendNumTicks: (legendNumTicks) => set({ legendNumTicks }),
  setTitle: (title) => set({ title }),
}));

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

type CameraState = {
  backgroundColorName: BackgroundColorName;
  particleSize: number;
  position: Vector3;
  projection: Projection;
  rotation: Vector3;
  zoom: number;
  setBackgroundColorName: (backgroundColorName: BackgroundColorName) => void;
  setParticleSize: (particleSize: number) => void;
  setPosition: (position: Vector3) => void;
  setProjection: (projection: Projection) => void;
  setRotation: (rotation: Vector3) => void;
  setZoom: (zoom: number) => void;
};

export const useCameraStore = create<CameraState>((set) => ({
  backgroundColorName: "none",
  particleSize: 10,
  position: new Vector3(0, 0, 0),
  projection: "orthographic",
  rotation: new Vector3(0, 0, 0),
  zoom: 1,
  setBackgroundColorName: (backgroundColorName) => set({ backgroundColorName }),
  setParticleSize: (particleSize) => set({ particleSize }),
  setPosition: (position) => set({ position: position.clone() }),
  setProjection: (projection) => set({ projection }),
  setRotation: (rotation) => set({ rotation: rotation.clone() }),
  setZoom: (zoom) => set({ zoom }),
}));

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
