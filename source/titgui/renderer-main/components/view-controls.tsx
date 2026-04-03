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
  Switch,
  Text,
} from "@radix-ui/themes";
import type { ComponentProps, ReactNode } from "react";
import { Activity, useState } from "react";
import {
  TbRefresh as AutoRangeIcon,
  TbBackground as BackgroundIcon,
  TbCheck as CheckIcon,
  TbChevronRight as CollapsedIcon,
  TbPalette as ColorMapIcon,
  TbChartDots as ComponentIcon,
  TbDroplet as DisplayIcon,
  TbChevronDown as ExpandedIcon,
  TbFileExport as ExportIcon,
  TbDatabase as FieldsIcon,
  TbCircle as FlatShadingIcon,
  TbPointer as InteractIcon,
  TbLassoPolygon as LassoIcon,
  TbMatrix as MatrixIcon,
  TbMathMax as MaxIcon,
  TbMathMin as MinIcon,
  TbPerspectiveOff as OrthographicIcon,
  TbPerspective as PerspectiveIcon,
  TbRulerMeasure as RangeIcon,
  TbRectangle as RectIcon,
  TbGrain as ScalarIcon,
  TbSun as ShadingIcon,
  TbArrowsHorizontal as SizeIcon,
  TbArrowsRandom as VectorIcon,
  TbEye as ViewIcon,
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
  FieldRank,
} from "~/renderer-common/visual/fields";
import type { GlyphScaleMode } from "~/renderer-common/visual/glyphs";
import type { ShadingMode } from "~/renderer-common/visual/particles";
import type { RenderMode } from "~/renderer-common/visual/particles-switch";
import { ExportButton } from "~/renderer-main/components/export";
import type { ToolMode } from "~/renderer-main/components/view-selection";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

type ViewControlsProps = ToolControlsProps &
  CameraControlsProps &
  DisplayControlsProps &
  RenderControlsProps;

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
  // Display.
  frameData,
  field,
  setFieldByName,
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
  // Render.
  shadingMode,
  setShadingMode,
  legendEnabled,
  setLegendEnabled,
  legendTickCount,
  setLegendTickCount,
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

      <Separator orientation="vertical" size="1" />

      {/* ---- Display. ---------------------------------------------------- */}
      <DisplayControls
        frameData={frameData}
        field={field}
        setFieldByName={setFieldByName}
        renderMode={renderMode}
        setRenderMode={setRenderMode}
        pointSize={pointSize}
        setPointSize={setPointSize}
        glyphScale={glyphScale}
        setGlyphScale={setGlyphScale}
        glyphScaleMode={glyphScaleMode}
        setGlyphScaleMode={setGlyphScaleMode}
        colorRangeMode={colorRangeMode}
        setColorRangeMode={setColorRangeMode}
        colorRange={colorRange}
        setColorRange={setColorRange}
        colorMapName={colorMapName}
        setColorMapName={setColorMapName}
        colorField={colorField}
        {...(renderMode === "glyphs" && { setColorFieldByName })}
        colorFieldModifier={colorFieldModifier}
        setColorFieldModifier={setColorFieldModifier}
      />

      <Separator orientation="vertical" size="1" />

      {/* ---- Render. ----------------------------------------------------- */}
      <RenderControls
        shadingMode={shadingMode}
        setShadingMode={setShadingMode}
        legendEnabled={legendEnabled}
        setLegendEnabled={setLegendEnabled}
        legendTickCount={legendTickCount}
        setLegendTickCount={setLegendTickCount}
      />

      <Separator orientation="vertical" size="1" />

      {/* ---- Export. ----------------------------------------------------- */}
      <ExportButton size="1" color="gray" variant="ghost">
        <Flex align="center" gap="1">
          <ExportIcon size={16} />
          Export…
        </Flex>
      </ExportButton>
    </Flex>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

interface ToolControlsProps {
  toolMode: ToolMode;
  setToolMode: (value: ToolMode) => void;
}

function ToolControls({ toolMode, setToolMode }: Readonly<ToolControlsProps>) {
  const toolModes = [
    { mode: "normal", label: "Navigate", icon: <InteractIcon size={16} /> },
    { mode: "rect", label: "Box Select", icon: <RectIcon size={16} /> },
    { mode: "lasso", label: "Lasso Select", icon: <LassoIcon size={16} /> },
  ] as const;
  return (
    <ControlsDropdown label="Interact" icon={<InteractIcon size={16} />}>
      {toolModes.map(({ mode, label, icon }) => (
        <DropdownMenu.Item
          key={mode}
          onClick={() => {
            setToolMode(mode);
          }}
        >
          <Flex align="center" justify="between" width="100%">
            <Flex align="center" gap="2">
              {icon}
              {label}
            </Flex>
            {toolMode === mode && <CheckIcon size={16} />}
          </Flex>
        </DropdownMenu.Item>
      ))}
    </ControlsDropdown>
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
    <ControlsDropdown label="View" icon={<ViewIcon size={16} />}>
      {/* ---- Projection. ------------------------------------------------- */}
      <Text size="1" color="gray">
        Projection
      </Text>
      <Select.Root
        size="1"
        value={projection}
        onValueChange={(value: Projection) => {
          setProjection(value);
        }}
      >
        <Select.Trigger />
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

      {/* ---- Background. ------------------------------------------------- */}
      <Text size="1" color="gray">
        Background Color
      </Text>
      <Select.Root
        size="1"
        value={backgroundColorName}
        onValueChange={(value: BackgroundColorName) => {
          setBackgroundColorName(value);
        }}
      >
        <Select.Trigger>
          <Flex align="center" gap="2">
            <BackgroundIcon size={16} />
            {backgroundColors[backgroundColorName].label}
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

      <ControlsSection label="Advanced Camera" defaultOpen={false}>
        <Flex direction="column" gap="1">
          {/* ---- Position. ----------------------------------------------- */}
          <ControlsSection label="Position">
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
          </ControlsSection>

          {/* ---- Rotation. ----------------------------------------------- */}
          <ControlsSection label="Rotation">
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
          </ControlsSection>
        </Flex>
      </ControlsSection>
    </ControlsDropdown>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

interface DisplayControlsProps {
  frameData: FieldMap;
  field: Field;
  setFieldByName: (value: string) => void;
  renderMode: RenderMode;
  setRenderMode: (value: RenderMode) => void;
  pointSize: number;
  setPointSize: (value: number) => void;
  glyphScale: number;
  setGlyphScale: (value: number) => void;
  glyphScaleMode: GlyphScaleMode;
  setGlyphScaleMode: (value: GlyphScaleMode) => void;
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

function DisplayControls({
  frameData,
  field,
  setFieldByName,
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
  colorRangeMode,
  setColorRangeMode,
  colorRange,
  setColorRange,
}: Readonly<DisplayControlsProps>) {
  return (
    <ControlsDropdown label="Display" icon={<DisplayIcon size={16} />}>
      <Flex direction="column" gap="2">
        <ControlsSection label="Display Mapping">
          {/* ---- Field. -------------------------------------------------- */}
          <Text size="1" color="gray">
            Field
          </Text>
          <Select.Root
            size="1"
            value={field.name}
            onValueChange={setFieldByName}
          >
            <Select.Trigger>
              <Flex align="center" gap="2">
                <FieldsIcon size={16} />
                <TechText>{field.name}</TechText>
              </Flex>
            </Select.Trigger>
            <Select.Content>
              {Array.from(frameData.entries()).map(([name, entry]) => (
                <Select.Item key={name} value={name}>
                  <Flex align="center" gap="2">
                    <FieldIcon rank={entry.rank} size={16} />
                    <TechText>{name}</TechText>
                  </Flex>
                </Select.Item>
              ))}
            </Select.Content>
          </Select.Root>

          {field.rank === 1 && (
            <>
              {/* ---- Representation. ------------------------------------- */}
              <Text size="1" color="gray">
                Representation
              </Text>
              <Select.Root
                size="1"
                value={renderMode}
                onValueChange={(value: RenderMode) => {
                  setRenderMode(value);
                }}
              >
                <Select.Trigger />
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
            </>
          )}

          {renderMode === "points" && (
            <>
              {/* ---- Point size. ----------------------------------------- */}
              <Text size="1" color="gray">
                Point Size
              </Text>
              <NumberEditor
                size="1"
                label={<SizeIcon size={16} />}
                min={0}
                value={pointSize}
                onValueChange={setPointSize}
              />
            </>
          )}

          {renderMode === "glyphs" && (
            <>
              {/* ---- Glyph scale mode. ----------------------------------- */}
              <Text size="1" color="gray">
                Glyph Scale Mode
              </Text>
              <Select.Root
                size="1"
                value={glyphScaleMode}
                onValueChange={(value: GlyphScaleMode) => {
                  setGlyphScaleMode(value);
                }}
              >
                <Select.Trigger />
                <Select.Content>
                  <Select.Item value="magnitude">
                    <Flex align="center" gap="2">
                      <RangeIcon size={16} />
                      Magnitude
                    </Flex>
                  </Select.Item>
                  <Select.Item value="uniform">
                    <Flex align="center" gap="2">
                      <SizeIcon size={16} />
                      Uniform
                    </Flex>
                  </Select.Item>
                </Select.Content>
              </Select.Root>

              {/* ---- Glyph scale. ---------------------------------------- */}
              <Text size="1" color="gray">
                Glyph Scale
              </Text>
              <NumberEditor
                size="1"
                label={<SizeIcon size={16} />}
                min={0}
                value={glyphScale}
                onValueChange={setGlyphScale}
              />
            </>
          )}
        </ControlsSection>

        <ControlsSection label="Coloring">
          {setColorFieldByName !== undefined && (
            <>
              {/* ---- Color by. ------------------------------------------- */}
              <Text size="1" color="gray">
                Color By
              </Text>
              <Select.Root
                size="1"
                value={colorField.name}
                onValueChange={setColorFieldByName}
              >
                <Select.Trigger />
                <Select.Content>
                  {Array.from(frameData.entries()).map(([name, entry]) => (
                    <Select.Item key={name} value={name}>
                      <Flex align="center" gap="2">
                        <FieldIcon rank={entry.rank} size={16} />
                        <TechText>{name}</TechText>
                      </Flex>
                    </Select.Item>
                  ))}
                </Select.Content>
              </Select.Root>
            </>
          )}

          {/* ---- Color range. -------------------------------------------- */}
          <Text size="1" color="gray">
            Color Range
          </Text>
          <Select.Root
            size="1"
            value={colorRangeMode}
            onValueChange={(value: ColorRangeMode) => {
              setColorRangeMode(value);
            }}
          >
            <Select.Trigger />
            <Select.Content>
              <Select.Item value="auto">
                <Flex align="center" gap="2">
                  <AutoRangeIcon size={16} />
                  Auto
                </Flex>
              </Select.Item>
              <Select.Item value="manual">
                <Flex align="center" gap="2">
                  <SizeIcon size={16} />
                  Manual
                </Flex>
              </Select.Item>
            </Select.Content>
          </Select.Root>

          {colorRangeMode === "manual" && (
            <Flex direction="row" gap="1" align="center">
              {/* ---- Min / Max. ------------------------------------------ */}
              <NumberEditor
                size="1"
                label={<MinIcon size={16} />}
                max={colorRange.max}
                value={colorRange.min}
                onValueChange={(value) => {
                  setColorRange({ ...colorRange, min: value });
                }}
              />
              <Text color="gray">-</Text>
              <NumberEditor
                size="1"
                label={<MaxIcon size={16} />}
                min={colorRange.min}
                value={colorRange.max}
                onValueChange={(value) => {
                  setColorRange({ ...colorRange, max: value });
                }}
              />
            </Flex>
          )}

          {/* ---- Color map. ---------------------------------------------- */}
          <Text size="1" color="gray">
            Color Map
          </Text>
          <Select.Root
            size="1"
            value={colorMapName}
            onValueChange={(value: ColorMapName) => {
              setColorMapName(value);
            }}
          >
            <Select.Trigger>
              <Flex align="center" gap="2">
                <ColorMapIcon size={16} />
                {colorMaps[colorMapName].label}
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
              {/* ---- Color metric. --------------------------------------- */}
              <Text size="1" color="gray">
                Color Metric
              </Text>
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
                <Select.Trigger />
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

              {/* ---- Component. ------------------------------------------ */}
              {typeof colorFieldModifier === "number" && (
                <>
                  <Text size="1" color="gray">
                    Component
                  </Text>
                  <Select.Root
                    size="1"
                    value={colorFieldModifier.toString()}
                    onValueChange={(value) => {
                      setColorFieldModifier(Number.parseInt(value, 10));
                    }}
                  >
                    <Select.Trigger />
                    <Select.Content>
                      {["x", "y", "z"]
                        .slice(0, colorField.dim)
                        .map((axis, i) => (
                          <Select.Item key={axis} value={i.toString()}>
                            <Flex align="center" gap="2">
                              <ComponentIcon size={16} />
                              <TechText>{axis}</TechText>
                            </Flex>
                          </Select.Item>
                        ))}
                    </Select.Content>
                  </Select.Root>
                </>
              )}
            </>
          )}

          {colorField.rank === 2 && (
            <>
              {/* ---- Color metric. --------------------------------------- */}
              <Text size="1" color="gray">
                Color Metric
              </Text>
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
                <Select.Trigger />
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

              {/* ---- Component. ------------------------------------------ */}
              {typeof colorFieldModifier === "number" && (
                <>
                  <Text size="1" color="gray">
                    Component
                  </Text>
                  <Select.Root
                    size="1"
                    value={colorFieldModifier.toString()}
                    onValueChange={(value) => {
                      setColorFieldModifier(Number.parseInt(value, 10));
                    }}
                  >
                    <Select.Trigger />
                    <Select.Content>
                      {(() => {
                        const axes = ["x", "y", "z"].slice(0, colorField.dim);
                        return axes.flatMap((row, i) =>
                          axes.map((col, j) => (
                            <Select.Item
                              key={`${row}${col}`}
                              value={(i * axes.length + j).toString()}
                            >
                              <Flex align="center" gap="2">
                                <ComponentIcon size={16} />
                                <TechText>{`${row}${col}`}</TechText>
                              </Flex>
                            </Select.Item>
                          )),
                        );
                      })()}
                    </Select.Content>
                  </Select.Root>
                </>
              )}
            </>
          )}
        </ControlsSection>
      </Flex>
    </ControlsDropdown>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

interface RenderControlsProps {
  shadingMode: ShadingMode;
  setShadingMode: (value: ShadingMode) => void;
  legendEnabled: boolean;
  setLegendEnabled: (value: boolean) => void;
  legendTickCount: number;
  setLegendTickCount: (value: number) => void;
}

function RenderControls({
  shadingMode,
  setShadingMode,
  legendEnabled,
  setLegendEnabled,
  legendTickCount,
  setLegendTickCount,
}: Readonly<RenderControlsProps>) {
  return (
    <ControlsDropdown label="Render" icon={<ShadingIcon size={16} />}>
      <Flex direction="column" gap="2">
        <ControlsSection label="Shading">
          {/* ---- Shading mode. ------------------------------------------- */}
          <Text size="1" color="gray">
            Shading Mode
          </Text>
          <Select.Root
            size="1"
            value={shadingMode}
            onValueChange={(value: ShadingMode) => {
              setShadingMode(value);
            }}
          >
            <Select.Trigger />
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
        </ControlsSection>

        <ControlsSection label="Legend">
          {/* ---- Show legend. -------------------------------------------- */}
          <Flex align="center" justify="between">
            <Text size="1" color="gray">
              Show Legend
            </Text>
            <Switch
              size="1"
              checked={legendEnabled}
              onCheckedChange={setLegendEnabled}
            />
          </Flex>

          {legendEnabled && (
            <>
              {/* ---- Tick count. ----------------------------------------- */}
              <Text size="1" color="gray">
                Tick Count
              </Text>
              <NumberEditor
                size="1"
                type="int"
                min={2}
                value={legendTickCount}
                onValueChange={setLegendTickCount}
              />
            </>
          )}
        </ControlsSection>
      </Flex>
    </ControlsDropdown>
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
  const [open, setOpen] = useState(false);
  return (
    <DropdownMenu.Root open={open} onOpenChange={setOpen}>
      <DropdownMenu.Trigger>
        <Button size="1" color="gray" variant="ghost">
          <Flex align="center" gap="1">
            {icon}
            {label}
          </Flex>
          <DropdownMenu.TriggerIcon />
        </Button>
      </DropdownMenu.Trigger>
      <DropdownMenu.Content size="1">
        <Flex direction="column" gap="2" p="1" width="360px">
          {children}
        </Flex>
      </DropdownMenu.Content>
    </DropdownMenu.Root>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

interface ControlsSectionProps {
  label: string;
  children: ReactNode;
  defaultOpen?: boolean;
}

function ControlsSection({
  label,
  children,
  defaultOpen = true,
}: Readonly<ControlsSectionProps>) {
  const chevronSize = 10;
  const [open, setOpen] = useState(defaultOpen);
  return (
    <Flex direction="column" gap="1">
      <Flex
        align="center"
        gap="1"
        onClick={() => {
          setOpen((current) => !current);
        }}
        className="w-full cursor-pointer text-left text-(--gray-11) hover:text-(--gray-12)"
      >
        {open ? (
          <ExpandedIcon size={chevronSize} />
        ) : (
          <CollapsedIcon size={chevronSize} />
        )}
        <Text size="1">{label}</Text>
      </Flex>
      <Activity mode={open ? "visible" : "hidden"}>
        <Flex
          className="border-l border-(--gray-6)"
          direction="column"
          gap="1"
          pl="2"
          ml={`calc(${chevronSize / 2}px - 0.5px)`}
        >
          {children}
        </Flex>
      </Activity>
    </Flex>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

type FieldIconProps = ComponentProps<typeof ScalarIcon> & {
  rank: FieldRank;
};

function FieldIcon({ rank, ...props }: Readonly<FieldIconProps>) {
  switch (rank) {
    case 0:
      return <ScalarIcon {...props} />;
    case 1:
      return <VectorIcon {...props} />;
    case 2:
      return <MatrixIcon {...props} />;
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
