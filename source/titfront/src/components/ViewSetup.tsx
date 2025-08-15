/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { PropertySections, PropertyTree } from "~/components/PropertyTree";
import { useCameraStore, useColorsStore } from "~/stores/ViewStore";
import type { Projection } from "~/visual/Camera";
import {
  type BackgroundColorType,
  backgroundColors,
} from "~/visual/BackroundColor";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export function VisualSetup() {
  const {
    backgroundColor,
    position,
    projection,
    rotation,
    setBackgroundColor,
    setPosition,
    setProjection,
    setRotation,
    setZoom,
    zoom,
  } = useCameraStore();

  const {
    colorMap,
    max,
    min,
    particleSize,
    setColorMap,
    setMinMax,
    setParticleSize,
  } = useColorsStore();

  return (
    <PropertySections>
      <PropertySections.Section name="Colors">
        <PropertyTree>
          <PropertyTree.Property
            type="float"
            name="Particle Size"
            value={particleSize}
            setValue={setParticleSize}
            min={1}
            max={100}
          />
          <PropertyTree.Property
            type="colormap"
            name="Color Map"
            value={colorMap}
            setValue={setColorMap}
          />
          <PropertyTree.Property
            type="float"
            name="Min Value"
            value={min}
            setValue={setMinMax.bind(null, min, max)}
          />
          <PropertyTree.Property
            type="float"
            name="Max Value"
            value={max}
            setValue={setMinMax.bind(null, min, max)}
          />
        </PropertyTree>
      </PropertySections.Section>
      <PropertySections.Section name="Camera">
        <PropertyTree>
          <PropertyTree.Property
            type="enum"
            name="Projection"
            value={projection}
            setValue={(x) => setProjection(x as Projection)}
            options={["orthographic", "perspective"]}
          />
          <PropertyTree.Property
            type="record"
            name="Position"
            unit="m"
            value={position}
            setValue={setPosition}
          />
          <PropertyTree.Property
            type="record"
            name="Rotation"
            unit="°"
            value={rotation}
            setValue={setRotation}
          />
          <PropertyTree.Property
            type="float"
            name="Zoom"
            value={zoom}
            setValue={setZoom}
            min={0.1}
          />
          <PropertyTree.Property
            type="enum"
            name="Background"
            value={backgroundColor}
            setValue={(x) => setBackgroundColor(x as BackgroundColorType)}
            options={Object.keys(backgroundColors)}
          />
        </PropertyTree>
      </PropertySections.Section>
    </PropertySections>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
