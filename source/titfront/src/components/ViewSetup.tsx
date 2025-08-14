/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { PropertySections, PropertyTree } from "~/components/PropertyTree";
import { useDataStore } from "~/stores/DataStore";
import {
  type ColorLegendLocation,
  type ColorRangeType,
  useCameraStore,
  useColorLegendStore,
  useFieldsStore,
} from "~/stores/ViewStore";
import {
  type BackgroundColorName,
  backgroundColors,
} from "~/visual/BackroundColor";
import type { Projection } from "~/visual/Camera";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export function ViewSetup() {
  const { currentDataSet } = useDataStore();
  const {
    field,
    colorMapName,
    colorRangeType,
    colorRangeMin,
    colorRangeMax,
    setField,
    setColorMapName,
    setColorRangeType,
    setColorRange,
  } = useFieldsStore();

  const {
    legendLocation,
    legendNumTicks,
    title,
    setLegendLocation,
    setLegendNumTicks,
    setTitle,
  } = useColorLegendStore();

  const {
    backgroundColorName,
    particleSize,
    position,
    projection,
    rotation,
    zoom,
    setBackgroundColorName,
    setParticleSize,
    setPosition,
    setProjection,
    setRotation,
    setZoom,
  } = useCameraStore();

  return (
    <PropertySections>
      <PropertySections.Section name="Fields">
        <PropertyTree>
          <PropertyTree.Property
            type="enum"
            name="Field"
            options={Object.keys(currentDataSet ?? {})}
            value={field}
            setValue={setField}
          />
          <PropertyTree.Property
            type="colormap"
            name="Color map"
            value={colorMapName}
            setValue={setColorMapName}
          />
          <PropertyTree.Property
            type="enum"
            name="Color range"
            options={["auto", "custom"]}
            value={colorRangeType}
            setValue={(x) => setColorRangeType(x as ColorRangeType)}
          />
          <PropertyTree.Property
            type="float"
            name="Color range min. value"
            value={colorRangeMin}
            setValue={(x) => setColorRange(x, colorRangeMax)}
            disabled={colorRangeType === "auto"}
            max={colorRangeMax}
          />
          <PropertyTree.Property
            type="float"
            name="Color range max. value"
            value={colorRangeMax}
            setValue={(x) => setColorRange(colorRangeMin, x)}
            disabled={colorRangeType === "auto"}
            min={colorRangeMin}
          />
        </PropertyTree>
      </PropertySections.Section>

      <PropertySections.Section name="Color legend">
        <PropertyTree>
          <PropertyTree.Property
            type="enum"
            name="Legend location"
            value={legendLocation}
            setValue={(x) => setLegendLocation(x as ColorLegendLocation)}
            options={["left", "bottom", "none"]}
          />
          <PropertyTree.Property
            type="int"
            name="Number of ticks"
            value={legendNumTicks}
            setValue={setLegendNumTicks}
            min={2}
            max={100}
          />
          <PropertyTree.Property
            type="string"
            name="Title"
            value={title ?? ""}
            setValue={setTitle}
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
            value={backgroundColorName}
            setValue={(x) => setBackgroundColorName(x as BackgroundColorName)}
            options={Object.keys(backgroundColors)}
          />
          <PropertyTree.Property
            type="float"
            name="Particle size"
            value={particleSize}
            setValue={setParticleSize}
            min={1}
            max={100}
          />
        </PropertyTree>
      </PropertySections.Section>
    </PropertySections>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
