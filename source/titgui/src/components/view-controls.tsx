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
  Text,
} from "@radix-ui/themes";
import {
  TbBackground as BackgroundIcon,
  TbCamera as CameraIcon,
  TbDroplet as ColoringIcon,
  TbPalette as ColorMapIcon,
  TbChartDots as ComponentIcon,
  TbBolt as EffectsIcon,
  TbDatabaseExport as ExportIcon,
  TbGalaxy as FieldIcon,
  TbVectorTriangle as GlyphScaleModeIcon,
  TbSettings as ModifierIcon,
  TbPerspectiveOff as OrthographicIcon,
  TbPerspective as PerspectiveIcon,
  TbRulerMeasure as RangeIcon,
  TbShape as RenderIcon,
  TbVector as RepresentationIcon,
  TbSun as ShadingIcon,
} from "react-icons/tb";
import { Euler, MathUtils, Vector3 } from "three";

import { TechText } from "~/components/basic";
import { chrome } from "~/components/classes";
import { ColorBox } from "~/components/color-bar";
import { ExportButton } from "~/components/export";
import { NumberEditor } from "~/components/number-editor";
import { toCSSColor } from "~/utils";
import {
  type BackgroundColorName,
  backgroundColors,
} from "~/visual/background-color";
import type { Projection } from "~/visual/camera";
import {
  type ColorMapName,
  type ColorRange,
  type ColorRangeMode,
  colorMaps,
} from "~/visual/color-map";
import type { EffectsSettings } from "~/visual/effects";
import type {
  Field,
  FieldMap,
  FieldModifier,
  FieldRank,
} from "~/visual/fields";
import type { GlyphScaleMode } from "~/visual/glyphs";
import type { ShadingMode } from "~/visual/particles";
import type { RenderMode } from "~/visual/particles-switch";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

type ViewControlsProps = {
  frameData: FieldMap;
  field: Field;
  setFieldByName: (value: string) => void;
} & CameraControlsProps &
  RenderControlsProps &
  ColorControlsProps &
  EffectsControlsProps;

export function ViewControls({
  frameData,
  field,
  setFieldByName,
  // Camera.
  projection,
  setProjection,
  backgroundColorName,
  setBackgroundColorName,
  cameraPosition,
  setCameraPosition,
  cameraRotation,
  setCameraRotation,
  // Render.
  renderMode,
  setRenderMode,
  shadingMode,
  setShadingMode,
  pointSize,
  setPointSize,
  glyphScale,
  setGlyphScale,
  glyphScaleMode,
  setGlyphScaleMode,
  // Color.
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
  // Effects.
  effects,
  setEffects,
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
              cameraPosition={cameraPosition}
              setCameraPosition={setCameraPosition}
              cameraRotation={cameraRotation}
              setCameraRotation={setCameraRotation}
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
              shadingMode={shadingMode}
              setShadingMode={setShadingMode}
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

      {/* ---- Effects. ---------------------------------------------------- */}
      <DropdownMenu.Root>
        <DropdownMenu.Trigger>
          <Button size="1" variant="ghost" className="rt-SelectTrigger">
            <Flex align="center" gap="1">
              <EffectsIcon size={16} />
              Effects
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
            <EffectsControls effects={effects} setEffects={setEffects} />
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
  cameraPosition: Vector3;
  setCameraPosition: (value: Vector3) => void;
  cameraRotation: Euler;
  setCameraRotation: (value: Euler) => void;
};

function CameraControls({
  projection,
  setProjection,
  backgroundColorName,
  setBackgroundColorName,
  cameraPosition,
  setCameraPosition,
  cameraRotation,
  setCameraRotation,
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

      <Flex direction="column" gap="1">
        <Text size="1" color="gray">
          Position
        </Text>
        <Flex direction="column" gap="1">
          {(["x", "y", "z"] as const).map((axis) => (
            <NumberEditor
              key={axis}
              size="1"
              label={axis.toUpperCase()}
              value={cameraPosition[axis]}
              onValueChange={(value) => {
                const next = new Vector3().copy(cameraPosition);
                next[axis] = value;
                setCameraPosition(next);
              }}
            />
          ))}
        </Flex>
      </Flex>

      <Flex direction="column" gap="1">
        <Text size="1" color="gray">
          Rotation
        </Text>
        <Flex direction="column" gap="1">
          {(["x", "y", "z"] as const).map((axis) => (
            <NumberEditor
              key={axis}
              size="1"
              label={axis.toUpperCase()}
              value={MathUtils.radToDeg(cameraRotation[axis])}
              onValueChange={(value) => {
                const next = new Euler().copy(cameraRotation);
                next[axis] = MathUtils.degToRad(value);
                setCameraRotation(next);
              }}
            />
          ))}
        </Flex>
      </Flex>
    </Flex>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

type RenderControlsProps = {
  field: Field;
  renderMode: RenderMode;
  setRenderMode: (value: RenderMode) => void;
  shadingMode: ShadingMode;
  setShadingMode: (value: ShadingMode) => void;
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
  shadingMode,
  setShadingMode,
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

      {/* ---- Shading mode. ----------------------------------------------- */}
      <Select.Root
        size="1"
        value={shadingMode}
        onValueChange={(value) => setShadingMode(value as ShadingMode)}
      >
        <Select.Trigger>
          <Flex align="center" gap="1">
            <ShadingIcon size={16} />
            Shading
          </Flex>
        </Select.Trigger>
        <Select.Content>
          <Select.Item value="flat">Flat</Select.Item>
          <Select.Item value="shaded">Shaded</Select.Item>
        </Select.Content>
      </Select.Root>

      {/* ---- Points mode. ------------------------------------------------ */}
      {renderMode === "points" && (
        <NumberEditor
          size="1"
          label="Point Size"
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
          <Text size="1" color="gray">
            Range
          </Text>
          <NumberEditor
            size="1"
            label="Min"
            max={colorRange.max}
            value={colorRange.min}
            onValueChange={(value) =>
              setColorRange({ ...colorRange, min: value })
            }
          />
          <NumberEditor
            size="1"
            label="Max"
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

type EffectsControlsProps = {
  effects: EffectsSettings;
  setEffects: (value: EffectsSettings) => void;
};

function EffectsControls({
  effects,
  setEffects,
}: Readonly<EffectsControlsProps>) {
  return (
    <Flex direction="column" gap="2">
      {/* ---- Eye-Dome Lighting. ------------------------------------------ */}
      <Text size="1" color="gray">
        Eye-Dome Lighting
      </Text>
      <Select.Root
        size="1"
        value={effects.edl.enabled ? "on" : "off"}
        onValueChange={(value) =>
          setEffects({
            ...effects,
            edl: { ...effects.edl, enabled: value === "on" },
          })
        }
      >
        <Select.Trigger>
          <Flex align="center" gap="1">
            <EffectsIcon size={16} />
            EDL
          </Flex>
        </Select.Trigger>
        <Select.Content>
          <Select.Item value="off">Disabled</Select.Item>
          <Select.Item value="on">Enabled</Select.Item>
        </Select.Content>
      </Select.Root>
      <NumberEditor
        disabled={!effects.edl.enabled}
        size="1"
        label="Radius"
        type="float"
        min={0.25}
        max={8}
        value={effects.edl.radius}
        onValueChange={(value) =>
          setEffects({
            ...effects,
            edl: { ...effects.edl, radius: value },
          })
        }
      />
      <NumberEditor
        disabled={!effects.edl.enabled}
        size="1"
        label="Strength"
        type="float"
        min={0.05}
        max={8}
        value={effects.edl.strength}
        onValueChange={(value) =>
          setEffects({
            ...effects,
            edl: { ...effects.edl, strength: value },
          })
        }
      />

      {/* ---- Depth Cueing. ----------------------------------------------- */}
      <Text size="1" color="gray">
        Depth Cueing
      </Text>
      <Select.Root
        size="1"
        value={effects.depthCue.enabled ? "on" : "off"}
        onValueChange={(value) =>
          setEffects({
            ...effects,
            depthCue: {
              ...effects.depthCue,
              enabled: value === "on",
            },
          })
        }
      >
        <Select.Trigger>
          <Flex align="center" gap="1">
            <EffectsIcon size={16} />
            Depth Cue
          </Flex>
        </Select.Trigger>
        <Select.Content>
          <Select.Item value="off">Disabled</Select.Item>
          <Select.Item value="on">Enabled</Select.Item>
        </Select.Content>
      </Select.Root>
      <NumberEditor
        disabled={!effects.depthCue.enabled}
        size="1"
        label="Strength"
        type="float"
        min={0.05}
        max={1}
        value={effects.depthCue.strength}
        onValueChange={(value) =>
          setEffects({
            ...effects,
            depthCue: { ...effects.depthCue, strength: value },
          })
        }
      />
      <NumberEditor
        disabled={!effects.depthCue.enabled}
        size="1"
        label="Exponent"
        type="float"
        min={0.25}
        max={4}
        value={effects.depthCue.exponent}
        onValueChange={(value) =>
          setEffects({
            ...effects,
            depthCue: { ...effects.depthCue, exponent: value },
          })
        }
      />

      {/* ---- Anti-Aliasing. ---------------------------------------------- */}
      <Text size="1" color="gray">
        Anti-Aliasing
      </Text>
      <Select.Root
        size="1"
        value={effects.smaa.enabled ? "on" : "off"}
        onValueChange={(value) =>
          setEffects({
            ...effects,
            smaa: { enabled: value === "on" },
          })
        }
      >
        <Select.Trigger>
          <Flex align="center" gap="1">
            <EffectsIcon size={16} />
            SMAA
          </Flex>
        </Select.Trigger>
        <Select.Content>
          <Select.Item value="off">Disabled</Select.Item>
          <Select.Item value="on">Enabled</Select.Item>
        </Select.Content>
      </Select.Root>
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
