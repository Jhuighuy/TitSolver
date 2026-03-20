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
import type { ComponentProps, ReactNode } from "react";
import {
  TbSparkles as AutoIcon,
  TbBackground as BackgroundIcon,
  TbCamera as CameraIcon,
  TbPalette as ColorMapIcon,
  TbDroplet as ColoringIcon,
  TbChartDots as ComponentIcon,
  TbDatabaseExport as ExportIcon,
  TbGalaxy as FieldIcon,
  TbCircle as FlatShadingIcon,
  TbLassoPolygon as LassoIcon,
  TbManualGearbox as ManualRangeIcon,
  TbMatrix as MatrixIcon,
  TbSettings as ModifierIcon,
  TbPointer as MouseIcon,
  TbPerspectiveOff as OrthographicIcon,
  TbPerspective as PerspectiveIcon,
  TbRulerMeasure as RangeIcon,
  TbRectangle as RectIcon,
  TbShape as RenderIcon,
  TbVector as RepresentationIcon,
  TbPoint as ScalarIcon,
  TbVectorTriangle as ScaleModeIcon,
  TbSun as ShadingIcon,
  TbTool as ToolIcon,
  TbArrowUpRight as VectorIcon,
} from "react-icons/tb";
import { Euler, MathUtils, Vector3 } from "three";

import { TechText } from "~/renderer-common/components/basic";
import { chrome } from "~/renderer-common/components/classes";
import { ColorBox } from "~/renderer-common/components/color-box";
import { NumberEditor } from "~/renderer-common/components/number-editor";
import { toCSSColor } from "~/renderer-common/utils";
import {
  type BackgroundColorName,
  backgroundColors,
} from "~/renderer-common/visual/background-color";
import type { Projection } from "~/renderer-common/visual/camera";
import {
  type ColorMapName,
  type ColorRange,
  type ColorRangeMode,
  colorMaps,
} from "~/renderer-common/visual/color-map";
import type {
  Field,
  FieldMap,
  FieldModifier,
} from "~/renderer-common/visual/fields";
import type { GlyphScaleMode } from "~/renderer-common/visual/glyphs";
import type { ShadingMode } from "~/renderer-common/visual/particles";
import type { RenderMode } from "~/renderer-common/visual/particles-switch";
import { ExportButton } from "~/renderer-main/components/export";
import type { ToolMode } from "~/renderer-main/components/view-selection";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

type ViewControlsProps = FieldControlsProps &
  ToolControlsProps &
  CameraControlsProps &
  RenderControlsProps &
  ColorControlsProps;

export function ViewControls({
  // Tool.
  toolMode,
  setToolMode,
  // Camera.
  projection,
  setProjection,
  backgroundColorName,
  setBackgroundColorName,
  cameraPosition,
  setCameraPosition,
  cameraRotation,
  setCameraRotation,
  // Field.
  frameData,
  field,
  setFieldByName,
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
      {/* ---- Tool. ------------------------------------------------------- */}
      <ToolControls toolMode={toolMode} setToolMode={setToolMode} />

      <Separator orientation="vertical" size="1" />

      {/* ---- Camera. ----------------------------------------------------- */}
      <ControlsDropdown label="Camera" icon={<CameraIcon size={16} />}>
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
      </ControlsDropdown>

      <Separator orientation="vertical" size="1" />

      {/* ---- Field. ------------------------------------------------------ */}
      <FieldControls
        frameData={frameData}
        field={field}
        setFieldByName={setFieldByName}
      />

      <Separator orientation="vertical" size="1" />

      {/* ---- Render. ----------------------------------------------------- */}
      <ControlsDropdown label="Render" icon={<RenderIcon size={16} />}>
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
      </ControlsDropdown>

      <Separator orientation="vertical" size="1" />

      {/* ---- Color range. ------------------------------------------------ */}
      <ControlsDropdown label="Coloring" icon={<ColoringIcon size={16} />}>
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
      </ControlsDropdown>

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

interface ControlsDropdownProps {
  label: string;
  icon: ReactNode;
  children: ReactNode;
}

function ControlsDropdown({
  label,
  icon,
  children,
}: Readonly<ControlsDropdownProps>) {
  return (
    <DropdownMenu.Root>
      <DropdownMenu.Trigger>
        <Button size="1" variant="ghost" className="rt-SelectTrigger">
          <Flex align="center" gap="1">
            {icon}
            {label}
          </Flex>
          <DropdownMenu.TriggerIcon className="rt-SelectIcon" />
        </Button>
      </DropdownMenu.Trigger>
      <DropdownMenu.Content>
        <Box
          px="2"
          py="2"
          minWidth="280px"
          onMouseDown={(event) => {
            event.stopPropagation();
          }}
          onClick={(event) => {
            event.stopPropagation();
          }}
        >
          {children}
        </Box>
      </DropdownMenu.Content>
    </DropdownMenu.Root>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

interface ToolControlsProps {
  toolMode: ToolMode;
  setToolMode: (value: ToolMode) => void;
}

function ToolControls({ toolMode, setToolMode }: Readonly<ToolControlsProps>) {
  return (
    <Select.Root
      size="1"
      value={toolMode}
      onValueChange={(value) => {
        setToolMode(value as ToolMode);
      }}
    >
      <Select.Trigger variant="ghost">
        <Flex align="center" gap="1">
          <ToolIcon size={16} />
          Tool
        </Flex>
      </Select.Trigger>
      <Select.Content>
        <Select.Item value="normal">
          <Flex align="center" gap="2">
            <MouseIcon size={16} />
            Navigate
          </Flex>
        </Select.Item>
        <Select.Item value="rect">
          <Flex align="center" gap="2">
            <RectIcon size={16} />
            Box Select
          </Flex>
        </Select.Item>
        <Select.Item value="lasso">
          <Flex align="center" gap="2">
            <LassoIcon size={16} />
            Lasso Select
          </Flex>
        </Select.Item>
      </Select.Content>
    </Select.Root>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

interface CameraControlsProps {
  projection: Projection;
  setProjection: (value: Projection) => void;
  backgroundColorName: BackgroundColorName;
  setBackgroundColorName: (value: BackgroundColorName) => void;
  cameraPosition: Vector3;
  setCameraPosition: (value: Vector3) => void;
  cameraRotation: Euler;
  setCameraRotation: (value: Euler) => void;
}

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
        onValueChange={(value: Projection) => {
          setProjection(value);
        }}
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
        onValueChange={(value: BackgroundColorName) => {
          setBackgroundColorName(value);
        }}
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

interface FieldControlsProps {
  frameData: FieldMap;
  field: Field;
  setFieldByName: (value: string) => void;
}

function FieldControls({
  frameData,
  field,
  setFieldByName,
}: Readonly<FieldControlsProps>) {
  return (
    <FieldSelect
      label="Field"
      variant="ghost"
      frameData={frameData}
      field={field}
      setFieldByName={setFieldByName}
    />
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

interface RenderControlsProps {
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
}

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
          onValueChange={(value: RenderMode) => {
            setRenderMode(value);
          }}
        >
          <Select.Trigger>
            <Flex align="center" gap="1">
              <RepresentationIcon size={16} />
              Representation
            </Flex>
          </Select.Trigger>
          <Select.Content>
            <Select.Item value="points">
              <Flex align="center" gap="2">
                <ScalarIcon size={16} />
                Points
              </Flex>
            </Select.Item>
            <Select.Item value="glyphs">
              <Flex align="center" gap="2">
                <VectorIcon size={16} />
                Glyphs
              </Flex>
            </Select.Item>
          </Select.Content>
        </Select.Root>
      )}

      {/* ---- Shading mode. ----------------------------------------------- */}
      <Select.Root
        size="1"
        value={shadingMode}
        onValueChange={(value: ShadingMode) => {
          setShadingMode(value);
        }}
      >
        <Select.Trigger>
          <Flex align="center" gap="1">
            <ShadingIcon size={16} />
            Shading
          </Flex>
        </Select.Trigger>
        <Select.Content>
          <Select.Item value="flat">
            <Flex align="center" gap="2">
              <FlatShadingIcon size={16} />
              Flat
            </Flex>
          </Select.Item>
          <Select.Item value="shaded">
            <Flex align="center" gap="2">
              <ShadingIcon size={16} />
              Shaded
            </Flex>
          </Select.Item>
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
            onValueChange={(value: GlyphScaleMode) => {
              setGlyphScaleMode(value);
            }}
          >
            <Select.Trigger>
              <Flex align="center" gap="1">
                <ScaleModeIcon size={16} />
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

interface ColorControlsProps {
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
}

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
        <FieldSelect
          label="Color By"
          frameData={frameData}
          field={colorField}
          setFieldByName={setColorFieldByName}
        />
      )}

      {/* ---- Color range. ------------------------------------------------ */}
      <Select.Root
        size="1"
        value={colorRangeMode}
        onValueChange={(value: ColorRangeMode) => {
          setColorRangeMode(value);
        }}
      >
        <Select.Trigger>
          <Flex align="center" gap="1">
            <RangeIcon size={16} />
            Color Range
          </Flex>
        </Select.Trigger>
        <Select.Content>
          <Select.Item value="auto">
            <Flex align="center" gap="2">
              <AutoIcon size={16} />
              Auto
            </Flex>
          </Select.Item>
          <Select.Item value="manual">
            <Flex align="center" gap="2">
              <ManualRangeIcon size={16} />
              Manual
            </Flex>
          </Select.Item>
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
            onValueChange={(value) => {
              setColorRange({ ...colorRange, min: value });
            }}
          />
          <NumberEditor
            size="1"
            label="Max"
            min={colorRange.min}
            value={colorRange.max}
            onValueChange={(value) => {
              setColorRange({ ...colorRange, max: value });
            }}
          />
        </>
      )}

      {/* ---- Color map. -------------------------------------------------- */}
      <Select.Root
        size="1"
        value={colorMapName}
        onValueChange={(value: ColorMapName) => {
          setColorMapName(value);
        }}
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
              <Select.Item value="magnitude">
                <Flex align="center" gap="2">
                  <RangeIcon size={16} />
                  Magnitude
                </Flex>
              </Select.Item>
              <Select.Item value="component">
                <Flex align="center" gap="2">
                  <ComponentIcon size={16} />
                  Component
                </Flex>
              </Select.Item>
            </Select.Content>
          </Select.Root>

          {typeof colorFieldModifier === "number" && (
            <Select.Root
              size="1"
              value={colorFieldModifier.toString()}
              onValueChange={(value) => {
                setColorFieldModifier(Number.parseInt(value, 10));
              }}
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
              <Select.Item value="determinant">
                <Flex align="center" gap="2">
                  <MatrixIcon size={16} />
                  Determinant
                </Flex>
              </Select.Item>
              <Select.Item value="component">
                <Flex align="center" gap="2">
                  <ComponentIcon size={16} />
                  Component
                </Flex>
              </Select.Item>
            </Select.Content>
          </Select.Root>

          {typeof colorFieldModifier === "number" && (
            <Select.Root
              size="1"
              value={colorFieldModifier.toString()}
              onValueChange={(value) => {
                setColorFieldModifier(Number.parseInt(value, 10));
              }}
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

interface FieldSelectProps {
  label: string;
  variant?: ComponentProps<typeof Select.Trigger>["variant"];
  frameData: FieldMap;
  field: Field;
  setFieldByName: (value: string) => void;
}

function FieldSelect({
  label,
  variant,
  frameData,
  field,
  setFieldByName,
}: Readonly<FieldSelectProps>) {
  return (
    <Select.Root size="1" value={field.name} onValueChange={setFieldByName}>
      <Select.Trigger {...(variant === undefined ? {} : { variant })}>
        <Flex align="center" gap="1">
          <FieldIcon size={16} />
          {label}
        </Flex>
      </Select.Trigger>
      <Select.Content>
        {Array.from(frameData.entries()).map(([name, field]) => (
          <Select.Item key={name} value={name}>
            <Flex align="center" gap="2">
              {(() => {
                switch (field.rank) {
                  case 0:
                    return <ScalarIcon size={16} />;
                  case 1:
                    return <VectorIcon size={16} />;
                  case 2:
                    return <MatrixIcon size={16} />;
                }
              })()}
              <TechText>{name}</TechText>
            </Flex>
          </Select.Item>
        ))}
      </Select.Content>
    </Select.Root>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
