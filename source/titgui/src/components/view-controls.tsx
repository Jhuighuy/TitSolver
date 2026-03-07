/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import {
  Box,
  Button,
  DropdownMenu,
  Flex,
  Select,
  Separator,
} from "@radix-ui/themes";
import {
  TbBackground as BackgroundIcon,
  TbCamera as CameraIcon,
  TbDroplet as ColoringIcon,
  TbPalette as ColorMapIcon,
  TbChartDots as ComponentIcon,
  TbDatabaseExport as ExportIcon,
  TbGalaxy as FieldIcon,
  TbPerspectiveOff as OrthographicIcon,
  TbPerspective as PerspectiveIcon,
  TbShape as RenderIcon,
  TbRulerMeasure as RangeIcon,
  TbSettings as ModifierIcon,
  TbVector as RepresentationIcon,
  TbVectorTriangle as GlyphScaleModeIcon,
} from "react-icons/tb";

import { TechText } from "~/components/basic";
import { chrome } from "~/components/classes";
import { ColorBox } from "~/components/color-bar";
import { ExportButton } from "~/components/export";
import { NumberEditor } from "~/components/number-editor";
import { toCSSColor } from "~/utils";
import type { GlyphScaleMode } from "~/visual/glyphs";
import {
  type BackgroundColorName,
  backgroundColors,
} from "~/visual/background-color";
import type { Projection } from "~/visual/camera";
import type {
  ColorMapName,
  ColorRangeMode,
  ColorRange,
} from "~/visual/color-map";
import { colorMaps } from "~/visual/color-map";
import type {
  Field,
  FieldMap,
  FieldModifier,
  FieldRank,
} from "~/visual/fields";
import type { RenderMode } from "~/visual/particles-switch";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

type ViewControlsProps = {
  frameData: FieldMap;
  field: Field;
  setFieldByName: (value: string) => void;
} & CameraControlsProps &
  RenderControlsProps &
  ColorControlsProps;

export function ViewControls({
  frameData,
  field,
  setFieldByName,
  projection,
  setProjection,
  backgroundColorName,
  setBackgroundColorName,
  renderMode,
  setRenderMode,
  pointSize,
  setPointSize,
  glyphScale,
  setGlyphScale,
  glyphScaleMode,
  setGlyphScaleMode,
  colorField,
  setColorFieldByName,
  colorFieldModifier,
  setColorFieldModifier,
  colorMapName,
  setColorMapName,
  colorRange,
  setColorRange,
  colorRangeMode,
  setColorRangeMode,
}: Readonly<ViewControlsProps>) {
  return (
    <Flex
      direction="row"
      align="center"
      height="36px"
      px="4"
      py="4px"
      gap="3"
      className={chrome({ direction: "br" })}
    >
      {/* ---- Camera. ----------------------------------------------------- */}
      <DropdownMenu.Root>
        <DropdownMenu.Trigger>
          <Button size="1" variant="ghost" className="rt-SelectTrigger">
            <Flex align="center" gap="1">
              <CameraIcon size={16} />
              Camera
            </Flex>
            <DropdownMenu.TriggerIcon className="rt-SelectIcon" />
          </Button>
        </DropdownMenu.Trigger>
        <DropdownMenu.Content>
          <Box
            px="2"
            py="2"
            style={{ minWidth: "240px" }}
            onMouseDown={(event) => event.stopPropagation()}
            onClick={(event) => event.stopPropagation()}
          >
            <CameraControls
              projection={projection}
              setProjection={setProjection}
              backgroundColorName={backgroundColorName}
              setBackgroundColorName={setBackgroundColorName}
            />
          </Box>
        </DropdownMenu.Content>
      </DropdownMenu.Root>

      <Separator orientation="vertical" size="1" />

      {/* ---- Field. ------------------------------------------------------ */}
      <Select.Root size="1" value={field.name} onValueChange={setFieldByName}>
        <Select.Trigger variant="ghost">
          <Flex align="center" gap="1">
            <FieldIcon size={16} />
            Field
          </Flex>
        </Select.Trigger>
        <Select.Content>
          {Array.from(frameData.entries()).map(([name, field]) => (
            <Select.Item key={name} value={name}>
              <Flex align="center" gap="2">
                <FieldTypeIcon rank={field.rank} />
                <TechText>{name}</TechText>
              </Flex>
            </Select.Item>
          ))}
        </Select.Content>
      </Select.Root>

      <Separator orientation="vertical" size="1" />

      {/* ---- Render. ----------------------------------------------------- */}
      <DropdownMenu.Root>
        <DropdownMenu.Trigger>
          <Button size="1" variant="ghost" className="rt-SelectTrigger">
            <Flex align="center" gap="1">
              <RenderIcon size={16} />
              Render
            </Flex>
            <DropdownMenu.TriggerIcon className="rt-SelectIcon" />
          </Button>
        </DropdownMenu.Trigger>
        <DropdownMenu.Content>
          <Box
            px="2"
            py="2"
            style={{ minWidth: "280px" }}
            onMouseDown={(event) => event.stopPropagation()}
            onClick={(event) => event.stopPropagation()}
          >
            <RenderControls
              field={field}
              renderMode={renderMode}
              setRenderMode={setRenderMode}
              pointSize={pointSize}
              setPointSize={setPointSize}
              glyphScale={glyphScale}
              setGlyphScale={setGlyphScale}
              glyphScaleMode={glyphScaleMode}
              setGlyphScaleMode={setGlyphScaleMode}
            />
          </Box>
        </DropdownMenu.Content>
      </DropdownMenu.Root>

      <Separator orientation="vertical" size="1" />

      {/* ---- Color range. ------------------------------------------------ */}
      <DropdownMenu.Root>
        <DropdownMenu.Trigger>
          <Button size="1" variant="ghost" className="rt-SelectTrigger">
            <Flex align="center" gap="1">
              <ColoringIcon size={16} />
              Coloring
            </Flex>
            <DropdownMenu.TriggerIcon className="rt-SelectIcon" />
          </Button>
        </DropdownMenu.Trigger>
        <DropdownMenu.Content>
          <Box
            px="2"
            py="2"
            style={{ minWidth: "280px" }}
            onMouseDown={(event) => event.stopPropagation()}
            onClick={(event) => event.stopPropagation()}
          >
            <ColorControls
              colorRangeMode={colorRangeMode}
              setColorRangeMode={setColorRangeMode}
              colorRange={colorRange}
              setColorRange={setColorRange}
              colorMapName={colorMapName}
              setColorMapName={setColorMapName}
              frameData={frameData}
              colorField={colorField}
              setColorFieldByName={setColorFieldByName}
              colorFieldModifier={colorFieldModifier}
              setColorFieldModifier={setColorFieldModifier}
            />
          </Box>
        </DropdownMenu.Content>
      </DropdownMenu.Root>

      <Separator orientation="vertical" size="1" />

      {/* ---- Export. ----------------------------------------------------- */}
      <ExportButton size="1" variant="ghost">
        <Flex align="center" gap="1">
          <ExportIcon size={16} />
          Export…
        </Flex>
      </ExportButton>
    </Flex>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

type CameraControlsProps = {
  projection: Projection;
  setProjection: (value: Projection) => void;
  backgroundColorName: BackgroundColorName;
  setBackgroundColorName: (value: BackgroundColorName) => void;
};

function CameraControls({
  projection,
  setProjection,
  backgroundColorName,
  setBackgroundColorName,
}: Readonly<CameraControlsProps>) {
  return (
    <Flex direction="column" gap="2">
      <Select.Root
        size="1"
        value={projection}
        onValueChange={(x) => setProjection(x as Projection)}
      >
        <Select.Trigger>
          <Flex align="center" gap="2">
            {projection === "perspective" ? (
              <PerspectiveIcon size={16} />
            ) : (
              <OrthographicIcon size={16} />
            )}
            Projection
          </Flex>
        </Select.Trigger>
        <Select.Content>
          <Select.Item value="perspective">
            <Flex align="center" gap="1">
              <PerspectiveIcon size={16} />
              Perspective
            </Flex>
          </Select.Item>
          <Select.Item value="orthographic">
            <Flex align="center" gap="2">
              <OrthographicIcon size={16} />
              Orthographic
            </Flex>
          </Select.Item>
        </Select.Content>
      </Select.Root>

      <Select.Root
        size="1"
        value={backgroundColorName}
        onValueChange={(x) => setBackgroundColorName(x as BackgroundColorName)}
      >
        <Select.Trigger>
          <Flex align="center" gap="1">
            <BackgroundIcon size={16} />
            Background
          </Flex>
        </Select.Trigger>
        <Select.Content>
          {Object.entries(backgroundColors).map(([name, { label, color }]) => (
            <Select.Item key={label} value={name}>
              <Flex align="center" gap="2">
                <Box
                  width="16px"
                  height="16px"
                  {...(color !== null && {
                    style: { backgroundColor: toCSSColor(color) },
                  })}
                />
                {label}
              </Flex>
            </Select.Item>
          ))}
        </Select.Content>
      </Select.Root>
    </Flex>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

type RenderControlsProps = {
  field: Field;
  renderMode: RenderMode;
  setRenderMode: (value: RenderMode) => void;
  pointSize: number;
  setPointSize: (value: number) => void;
  glyphScale: number;
  setGlyphScale: (value: number) => void;
  glyphScaleMode: GlyphScaleMode;
  setGlyphScaleMode: (value: GlyphScaleMode) => void;
};

function RenderControls({
  field,
  renderMode,
  setRenderMode,
  pointSize,
  setPointSize,
  glyphScale,
  setGlyphScale,
  glyphScaleMode,
  setGlyphScaleMode,
}: Readonly<RenderControlsProps>) {
  return (
    <Flex direction="column" gap="2">
      {/* ---- Visual mode. ------------------------------------------------ */}
      {field.rank === 1 && (
        <Select.Root
          size="1"
          value={renderMode}
          onValueChange={(value) => setRenderMode(value as RenderMode)}
        >
          <Select.Trigger>
            <Flex align="center" gap="1">
              <RepresentationIcon size={16} />
              Representation
            </Flex>
          </Select.Trigger>
          <Select.Content>
            <Select.Item value="points">Points</Select.Item>
            <Select.Item value="glyphs">Glyphs</Select.Item>
          </Select.Content>
        </Select.Root>
      )}

      {/* ---- Points mode. ------------------------------------------------ */}
      {renderMode === "points" && (
        <NumberEditor
          size="1"
          label="Point Size"
          type="float"
          min={0.001}
          value={pointSize}
          onValueChange={setPointSize}
        />
      )}

      {/* ---- Glyphs mode. ------------------------------------------------ */}
      {renderMode === "glyphs" && (
        <>
          <NumberEditor
            size="1"
            label="Scale"
            type="float"
            min={0.001}
            value={glyphScale}
            onValueChange={setGlyphScale}
          />

          <Select.Root
            size="1"
            value={glyphScaleMode}
            onValueChange={(value) =>
              setGlyphScaleMode(value as GlyphScaleMode)
            }
          >
            <Select.Trigger>
              <Flex align="center" gap="1">
                <GlyphScaleModeIcon size={16} />
                Scale Mode
              </Flex>
            </Select.Trigger>
            <Select.Content>
              <Select.Item value="magnitude">Magnitude</Select.Item>
              <Select.Item value="uniform">Uniform</Select.Item>
            </Select.Content>
          </Select.Root>
        </>
      )}
    </Flex>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

type ColorControlsProps = {
  frameData: FieldMap;
  colorField: Field;
  setColorFieldByName?: (value: string) => void;
  colorFieldModifier: FieldModifier;
  setColorFieldModifier: (value: FieldModifier) => void;
  colorMapName: ColorMapName;
  setColorMapName: (value: ColorMapName) => void;
  colorRangeMode: ColorRangeMode;
  setColorRangeMode: (value: ColorRangeMode) => void;
  colorRange: ColorRange;
  setColorRange: (value: ColorRange) => void;
};

function ColorControls({
  frameData,
  colorField,
  setColorFieldByName,
  colorFieldModifier,
  setColorFieldModifier,
  colorMapName,
  setColorMapName,
  colorRangeMode,
  setColorRangeMode,
  colorRange,
  setColorRange,
}: Readonly<ColorControlsProps>) {
  return (
    <Flex direction="column" gap="2">
      {setColorFieldByName !== undefined && (
        <Select.Root
          size="1"
          value={colorField.name}
          onValueChange={setColorFieldByName}
        >
          <Select.Trigger>
            <Flex align="center" gap="1">
              <FieldIcon size={16} />
              Color By
            </Flex>
          </Select.Trigger>
          <Select.Content>
            {Array.from(frameData.entries()).map(([name, field]) => (
              <Select.Item key={name} value={name}>
                <Flex align="center" gap="2">
                  <FieldTypeIcon rank={field.rank} />
                  <TechText>{name}</TechText>
                </Flex>
              </Select.Item>
            ))}
          </Select.Content>
        </Select.Root>
      )}

      {/* ---- Color range. ------------------------------------------------ */}
      <Select.Root
        size="1"
        value={colorRangeMode}
        onValueChange={(value) => setColorRangeMode(value as ColorRangeMode)}
      >
        <Select.Trigger>
          <Flex align="center" gap="1">
            <RangeIcon size={16} />
            Color Range
          </Flex>
        </Select.Trigger>
        <Select.Content>
          <Select.Item value="auto">Auto</Select.Item>
          <Select.Item value="manual">Manual</Select.Item>
        </Select.Content>
      </Select.Root>

      {colorRangeMode === "manual" && (
        <>
          <NumberEditor
            size="1"
            label="Min"
            type="float"
            max={colorRange.max}
            value={colorRange.min}
            onValueChange={(value) =>
              setColorRange({ ...colorRange, min: value })
            }
          />
          <NumberEditor
            size="1"
            label="Max"
            type="float"
            min={colorRange.min}
            value={colorRange.max}
            onValueChange={(value) =>
              setColorRange({ ...colorRange, max: value })
            }
          />
        </>
      )}

      {/* ---- Color map. -------------------------------------------------- */}
      <Select.Root
        size="1"
        value={colorMapName}
        onValueChange={(x) => setColorMapName(x as ColorMapName)}
      >
        <Select.Trigger>
          <Flex align="center" gap="1">
            <ColorMapIcon size={16} />
            Color Map
          </Flex>
        </Select.Trigger>
        <Select.Content>
          {Object.entries(colorMaps).map(([name, { label }]) => (
            <Select.Item key={label} value={name}>
              <Flex align="center" gap="2">
                <ColorBox
                  name={name as ColorMapName}
                  width="64px"
                  height="16px"
                />
                {label}
              </Flex>
            </Select.Item>
          ))}
        </Select.Content>
      </Select.Root>

      {colorField.rank === 1 && (
        <>
          {/* ---- Vector field modifier. ---------------------------------- */}
          <Select.Root
            size="1"
            value={
              typeof colorFieldModifier === "number"
                ? "component"
                : colorFieldModifier
            }
            onValueChange={(value) =>
              setColorFieldModifier(
                (value === "component" ? 0 : value) as FieldModifier,
              )
            }
          >
            <Select.Trigger>
              <Flex align="center" gap="1">
                <ModifierIcon size={16} />
                Color Metric
              </Flex>
            </Select.Trigger>
            <Select.Content>
              <Select.Item value="magnitude">Magnitude</Select.Item>
              <Select.Item value="component">Component</Select.Item>
            </Select.Content>
          </Select.Root>

          {typeof colorFieldModifier === "number" && (
            <Select.Root
              size="1"
              value={colorFieldModifier.toString()}
              onValueChange={(value) =>
                setColorFieldModifier(Number.parseInt(value, 10))
              }
            >
              <Select.Trigger>
                <Flex align="center" gap="1">
                  <ComponentIcon size={16} />
                  Component
                </Flex>
              </Select.Trigger>
              <Select.Content>
                {["x", "y", "z"].slice(0, colorField.dim).map((axis, i) => (
                  <Select.Item key={axis} value={i.toString()}>
                    <TechText>{axis}</TechText>
                  </Select.Item>
                ))}
              </Select.Content>
            </Select.Root>
          )}
        </>
      )}

      {colorField.rank === 2 && (
        <>
          {/* ---- Matrix field modifier. ---------------------------------- */}
          <Select.Root
            size="1"
            value={
              typeof colorFieldModifier === "number"
                ? "component"
                : colorFieldModifier
            }
            onValueChange={(value) => {
              setColorFieldModifier(
                (value === "component" ? 0 : value) as FieldModifier,
              );
            }}
          >
            <Select.Trigger>
              <Flex align="center" gap="1">
                <ModifierIcon size={16} />
                Color Metric
              </Flex>
            </Select.Trigger>
            <Select.Content>
              <Select.Item value="determinant">Determinant</Select.Item>
              <Select.Item value="component">Component</Select.Item>
            </Select.Content>
          </Select.Root>

          {typeof colorFieldModifier === "number" && (
            <Select.Root
              size="1"
              value={colorFieldModifier.toString()}
              onValueChange={(value) =>
                setColorFieldModifier(Number.parseInt(value, 10))
              }
            >
              <Select.Trigger>
                <Flex align="center" gap="1">
                  <ComponentIcon size={16} />
                  Component
                </Flex>
              </Select.Trigger>
              <Select.Content>
                {(() => {
                  const axes = ["x", "y", "z"].slice(0, colorField.dim);
                  return axes.flatMap((row, i) =>
                    axes.map((col, j) => (
                      <Select.Item
                        key={`${row}${col}`}
                        value={(i * axes.length + j).toString()}
                      >
                        <TechText>{`${row}${col}`}</TechText>
                      </Select.Item>
                    )),
                  );
                })()}
              </Select.Content>
            </Select.Root>
          )}
        </>
      )}
    </Flex>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

type FieldTypeIconProps = {
  rank: FieldRank;
};

function FieldTypeIcon({ rank }: Readonly<FieldTypeIconProps>) {
  const label = ["S", "V", "M"][rank];
  return (
    <Box
      width="16px"
      height="16px"
      style={{
        border: "1px solid currentColor",
        display: "grid",
        placeItems: "center",
        opacity: 0.8,
      }}
    >
      <TechText style={{ fontSize: "10px", lineHeight: 1 }}>{label}</TechText>
    </Box>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
