/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { Vector3 } from "three";
import { create } from "zustand";

import { Projection } from "~/visual/Camera";
import { BackgroundColorType } from "~/visual/BackroundColor";
import type { ColorMapType } from "~/visual/ColorMap";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

type ColorsState = {
  colorMap: ColorMapType;
  setColorMap: (colorMap: ColorMapType) => void;
  min: number;
  max: number;
  setMinMax: (min: number, max: number) => void;
  particleSize: number;
  setParticleSize: (size: number) => void;
};

export const useColorsStore = create<ColorsState>((set) => ({
  colorMap: "turbo",
  setColorMap(colorMap: ColorMapType) {
    set({ colorMap });
  },
  min: 0,
  max: 1,
  setMinMax(min: number, max: number) {
    set({ min, max });
  },
  particleSize: 10,
  setParticleSize(size: number) {
    set({ particleSize: size });
  },
}));

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

type CameraState = {
  projection: Projection;
  setProjection: (projection: Projection) => void;
  position: Vector3;
  setPosition: (position: Vector3) => void;
  rotation: Vector3;
  setRotation: (rotation: Vector3) => void;
  zoom: number;
  setZoom: (zoom: number) => void;
  backgroundColor: BackgroundColorType;
  setBackgroundColor: (backgroundColor: BackgroundColorType) => void;
};

export const useCameraStore = create<CameraState>((set) => ({
  projection: "orthographic",
  setProjection(projection: Projection) {
    set({ projection });
  },
  position: new Vector3(0, 0, 0),
  setPosition(position: Vector3) {
    set({ position: position.clone() });
  },
  rotation: new Vector3(0, 0, 0),
  setRotation(rotation: Vector3) {
    set({ rotation: rotation.clone() });
  },
  zoom: 1,
  setZoom(zoom: number) {
    set({ zoom });
  },
  backgroundColor: "none",
  setBackgroundColor(backgroundColor: BackgroundColorType) {
    set({ backgroundColor });
  },
}));

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
