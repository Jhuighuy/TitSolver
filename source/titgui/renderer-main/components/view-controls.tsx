/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import {
  IconArrowsHorizontal,
  IconRefresh as IconAutoRange,
  IconBackground,
  IconCheck,
  IconChartDots as IconComponent,
  IconDroplet,
  IconEye,
  IconFileExport,
  IconLassoPolygon,
  IconMathMax,
  IconMathMin,
  IconMatrix,
  IconPalette,
  IconPerspective,
  IconPerspectiveOff,
  IconPointer,
  IconRectangle,
  IconRulerMeasure,
  IconGrain as IconScalar,
  IconSun,
  IconSunOff,
  IconBaselineDensitySmall as IconTicks,
  IconArrowsRandom as IconVector,
} from "@tabler/icons-react";
import type { ComponentProps } from "react";
import { Euler, MathUtils, Vector3 } from "three";

import { chrome } from "~/renderer-common/components/classes";
import { ColorBox } from "~/renderer-common/components/color-box";
import { DropdownMenu } from "~/renderer-common/components/dropdown-menu";
import { NumberInput } from "~/renderer-common/components/input";
import { Box, Flex } from "~/renderer-common/components/layout";
import { Section } from "~/renderer-common/components/section";
import { Select } from "~/renderer-common/components/select";
import { Separator } from "~/renderer-common/components/separator";
import { Switch } from "~/renderer-common/components/switch";
import { Mono, Text } from "~/renderer-common/components/text";
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
      align="center"
      gap="1"
      height="9"
      minHeight="9"
      maxHeight="9"
      px="2"
      py="1"
      className={chrome()}
    >
      {/* ---- Tool. ------------------------------------------------------- */}
      <ToolControls toolMode={toolMode} setToolMode={setToolMode} />

      <Separator orientation="vertical" />

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

      <Separator orientation="vertical" />

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

      <Separator orientation="vertical" />

      {/* ---- Render. ----------------------------------------------------- */}
      <RenderControls
        shadingMode={shadingMode}
        setShadingMode={setShadingMode}
        legendEnabled={legendEnabled}
        setLegendEnabled={setLegendEnabled}
        legendTickCount={legendTickCount}
        setLegendTickCount={setLegendTickCount}
      />

      <Separator orientation="vertical" />

      {/* ---- Export. ----------------------------------------------------- */}
      <ExportButton variant="ghost">
        <IconFileExport />
        Export…
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
    { mode: "normal", label: "Navigate", icon: <IconPointer /> },
    { mode: "rect", label: "Box Select", icon: <IconRectangle /> },
    {
      mode: "lasso",
      label: "Lasso Select",
      icon: <IconLassoPolygon />,
    },
  ] as const;
  return (
    <DropdownMenu.Root>
      <DropdownMenu.Trigger>
        <IconPointer />
        Interact
      </DropdownMenu.Trigger>
      <DropdownMenu.Content>
        {toolModes.map(({ mode, label, icon }) => (
          <DropdownMenu.Item
            key={mode}
            className="justify-between"
            onClick={() => {
              setToolMode(mode);
            }}
          >
            <Flex align="center" gap="2">
              {icon}
              {label}
            </Flex>
            {toolMode === mode && <IconCheck className="ml-auto" />}
          </DropdownMenu.Item>
        ))}
      </DropdownMenu.Content>
    </DropdownMenu.Root>
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
    <DropdownMenu.Root>
      <DropdownMenu.Trigger>
        <IconEye />
        View
      </DropdownMenu.Trigger>
      <DropdownMenu.Content className="w-90">
        {/* ---- Projection. ----------------------------------------------- */}
        <Text color="subtle">Projection</Text>
        <Select.Root value={projection} onValueChange={setProjection}>
          <Select.Trigger />
          <Select.Content>
            <Select.Item value="perspective">
              <IconPerspective />
              Perspective
            </Select.Item>
            <Select.Item value="orthographic">
              <IconPerspectiveOff />
              Orthographic
            </Select.Item>
          </Select.Content>
        </Select.Root>

        {/* ---- Background. ----------------------------------------------- */}
        <Text color="subtle">Background Color</Text>
        <Select.Root
          value={backgroundColorName}
          onValueChange={setBackgroundColorName}
        >
          <Select.Trigger>
            <IconBackground />
            {backgroundColors[backgroundColorName].label}
          </Select.Trigger>
          <Select.Content>
            {Object.entries(backgroundColors).map(
              ([name, { label, color }]) => (
                <Select.Item key={label} value={name}>
                  <Box
                    size="4"
                    {...(color === null
                      ? {
                          className: chrome(),
                        }
                      : {
                          style: { backgroundColor: toCSSColor(color) },
                        })}
                  />
                  {label}
                </Select.Item>
              ),
            )}
          </Select.Content>
        </Select.Root>

        <Section label="Advanced Camera" defaultOpen={false}>
          {/* ---- Position. ----------------------------------------------- */}
          <Text color="subtle">Position</Text>
          {(["x", "y", "z"] as const).map((axis) => (
            <NumberInput
              key={axis}
              slot={axis.toUpperCase()}
              value={cameraPosition[axis]}
              onValueChange={(value) => {
                const next = new Vector3().copy(cameraPosition);
                next[axis] = value;
                setCameraPosition(next);
              }}
            />
          ))}

          {/* ---- Rotation. ----------------------------------------------- */}
          <Text color="subtle">Rotation</Text>
          {(["x", "y", "z"] as const).map((axis) => (
            <NumberInput
              key={axis}
              slot={axis.toUpperCase()}
              value={MathUtils.radToDeg(cameraRotation[axis])}
              onValueChange={(value) => {
                const next = new Euler().copy(cameraRotation);
                next[axis] = MathUtils.degToRad(value);
                setCameraRotation(next);
              }}
            />
          ))}
        </Section>
      </DropdownMenu.Content>
    </DropdownMenu.Root>
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
    <DropdownMenu.Root>
      <DropdownMenu.Trigger>
        <IconDroplet />
        Display
      </DropdownMenu.Trigger>
      <DropdownMenu.Content className="w-90">
        <Section label="Display Mapping">
          {/* ---- Field. -------------------------------------------------- */}
          <Text color="subtle">Field</Text>
          <Select.Root value={field.name} onValueChange={setFieldByName}>
            <Select.Trigger />
            <Select.Content>
              {Array.from(frameData.entries()).map(([name, entry]) => (
                <Select.Item key={name} value={name}>
                  <FieldIcon rank={entry.rank} />
                  <Mono>{name}</Mono>
                </Select.Item>
              ))}
            </Select.Content>
          </Select.Root>

          {field.rank === 1 && (
            <>
              {/* ---- Representation. ------------------------------------- */}
              <Text color="subtle">Representation</Text>
              <Select.Root value={renderMode} onValueChange={setRenderMode}>
                <Select.Trigger />
                <Select.Content>
                  <Select.Item value="points">
                    <IconScalar />
                    Points
                  </Select.Item>
                  <Select.Item value="glyphs">
                    <IconVector />
                    Glyphs
                  </Select.Item>
                </Select.Content>
              </Select.Root>
            </>
          )}

          {renderMode === "points" && (
            <>
              {/* ---- Point size. ----------------------------------------- */}
              <Text color="subtle">Point Size</Text>
              <NumberInput
                slot={<IconArrowsHorizontal />}
                min={0}
                value={pointSize}
                onValueChange={setPointSize}
              />
            </>
          )}

          {renderMode === "glyphs" && (
            <>
              {/* ---- Glyph scale mode. ----------------------------------- */}
              <Text color="subtle">Glyph Scale Mode</Text>
              <Select.Root
                value={glyphScaleMode}
                onValueChange={setGlyphScaleMode}
              >
                <Select.Trigger />
                <Select.Content>
                  <Select.Item value="magnitude">
                    <IconRulerMeasure />
                    Magnitude
                  </Select.Item>
                  <Select.Item value="uniform">
                    <IconArrowsHorizontal />
                    Uniform
                  </Select.Item>
                </Select.Content>
              </Select.Root>

              {/* ---- Glyph scale. ---------------------------------------- */}
              <Text color="subtle">Glyph Scale</Text>
              <NumberInput
                slot={<IconArrowsHorizontal />}
                min={0}
                value={glyphScale}
                onValueChange={setGlyphScale}
              />
            </>
          )}
        </Section>

        <Section label="Coloring">
          {setColorFieldByName !== undefined && (
            <>
              {/* ---- Color by. ------------------------------------------- */}
              <Text color="subtle">Color By</Text>
              <Select.Root
                value={colorField.name}
                onValueChange={setColorFieldByName}
              >
                <Select.Trigger />
                <Select.Content>
                  {Array.from(frameData.entries()).map(([name, entry]) => (
                    <Select.Item key={name} value={name}>
                      <FieldIcon rank={entry.rank} />
                      <Mono>{name}</Mono>
                    </Select.Item>
                  ))}
                </Select.Content>
              </Select.Root>
            </>
          )}

          {/* ---- Color range. -------------------------------------------- */}
          <Text color="subtle">Color Range</Text>
          <Select.Root value={colorRangeMode} onValueChange={setColorRangeMode}>
            <Select.Trigger />
            <Select.Content>
              <Select.Item value="auto">
                <IconAutoRange />
                Auto
              </Select.Item>
              <Select.Item value="manual">
                <IconArrowsHorizontal />
                Manual
              </Select.Item>
            </Select.Content>
          </Select.Root>

          {colorRangeMode === "manual" && (
            <Flex align="center" gap="1">
              {/* ---- Min / Max. ------------------------------------------ */}
              <NumberInput
                slot={<IconMathMin />}
                max={colorRange.max}
                value={colorRange.min}
                onValueChange={(value) => {
                  setColorRange({ ...colorRange, min: value });
                }}
              />
              <Text color="subtle">-</Text>
              <NumberInput
                slot={<IconMathMax />}
                min={colorRange.min}
                value={colorRange.max}
                onValueChange={(value) => {
                  setColorRange({ ...colorRange, max: value });
                }}
              />
            </Flex>
          )}

          {/* ---- Color map. ---------------------------------------------- */}
          <Text color="subtle">Color Map</Text>
          <Select.Root value={colorMapName} onValueChange={setColorMapName}>
            <Select.Trigger>
              <IconPalette />
              {colorMaps[colorMapName].label}
            </Select.Trigger>
            <Select.Content>
              {Object.entries(colorMaps).map(([name, { label }]) => (
                <Select.Item key={label} value={name}>
                  <ColorBox name={name as ColorMapName} className="h-4 w-16" />
                  {label}
                </Select.Item>
              ))}
            </Select.Content>
          </Select.Root>

          {colorField.rank === 1 && (
            <>
              {/* ---- Color metric. --------------------------------------- */}
              <Text color="subtle">Color Metric</Text>
              <Select.Root
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
                    <IconRulerMeasure />
                    Magnitude
                  </Select.Item>
                  <Select.Item value="component">
                    <IconComponent />
                    Component
                  </Select.Item>
                </Select.Content>
              </Select.Root>

              {/* ---- Component. ------------------------------------------ */}
              {typeof colorFieldModifier === "number" && (
                <>
                  <Text color="subtle">Component</Text>
                  <Select.Root
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
                            <IconComponent />
                            <Mono>{axis}</Mono>
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
              <Text color="subtle">Color Metric</Text>
              <Select.Root
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
                    <IconMatrix />
                    Determinant
                  </Select.Item>
                  <Select.Item value="component">
                    <IconComponent />
                    Component
                  </Select.Item>
                </Select.Content>
              </Select.Root>

              {/* ---- Component. ------------------------------------------ */}
              {typeof colorFieldModifier === "number" && (
                <>
                  <Text color="subtle">Component</Text>
                  <Select.Root
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
                              <IconComponent />
                              <Mono>{`${row}${col}`}</Mono>
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
        </Section>
      </DropdownMenu.Content>
    </DropdownMenu.Root>
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
    <DropdownMenu.Root>
      <DropdownMenu.Trigger>
        <IconSun />
        Render
      </DropdownMenu.Trigger>
      <DropdownMenu.Content className="w-90">
        <Section label="Shading">
          {/* ---- Shading mode. ------------------------------------------- */}
          <Text color="subtle">Shading Mode</Text>
          <Select.Root value={shadingMode} onValueChange={setShadingMode}>
            <Select.Trigger />
            <Select.Content>
              <Select.Item value="flat">
                <IconSunOff />
                Flat
              </Select.Item>
              <Select.Item value="shaded">
                <IconSun />
                Shaded
              </Select.Item>
            </Select.Content>
          </Select.Root>
        </Section>

        <Section label="Legend">
          {/* ---- Show legend. -------------------------------------------- */}
          <Flex align="center" justify="between">
            <Text color="subtle">Show Legend</Text>
            <Switch
              checked={legendEnabled}
              onCheckedChange={setLegendEnabled}
            />
          </Flex>

          {legendEnabled && (
            <>
              {/* ---- Tick count. ----------------------------------------- */}
              <Text color="subtle">Tick Count</Text>
              <NumberInput
                type="int"
                min={2}
                max={10}
                slot={<IconTicks />}
                value={legendTickCount}
                onValueChange={setLegendTickCount}
              />
            </>
          )}
        </Section>
      </DropdownMenu.Content>
    </DropdownMenu.Root>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

type FieldIconProps = ComponentProps<typeof IconScalar> & {
  rank: FieldRank;
};

function FieldIcon({ rank, ...props }: Readonly<FieldIconProps>) {
  switch (rank) {
    case 0:
      return <IconScalar {...props} />;
    case 1:
      return <IconVector {...props} />;
    case 2:
      return <IconMatrix {...props} />;
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
