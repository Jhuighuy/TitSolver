/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import {
  IconArrowsHorizontal,
  IconRefresh as IconAutoRange,
  IconCheck,
  IconChartDots as IconComponent,
  IconDroplet,
  IconEye,
  IconFileExport,
  IconLassoPolygon,
  IconMathMax,
  IconMathMin,
  IconMatrix,
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
import { useAtom, useAtomValue, useSetAtom } from "jotai";
import type { ComponentProps } from "react";
import { Euler, MathUtils, Vector3 } from "three";

import { surface } from "~/renderer/common/components/classes";
import { ColorBox } from "~/renderer/common/components/color-box";
import { DropdownMenu } from "~/renderer/common/components/dropdown-menu";
import { NumberInput } from "~/renderer/common/components/input";
import { Section } from "~/renderer/common/components/section";
import { Select, type SelectOption } from "~/renderer/common/components/select";
import { Separator } from "~/renderer/common/components/separator";
import { Switch } from "~/renderer/common/components/switch";
import { Mono, Text } from "~/renderer/common/components/text";
import { cn } from "~/renderer/common/components/utils";
import { toCSSColor } from "~/renderer/common/utils";
import {
  type BackgroundColorName,
  backgroundColors,
} from "~/renderer/common/visual/background-color";
import type { Projection } from "~/renderer/common/visual/camera";
import {
  type ColorMapName,
  type ColorRangeMode,
  colorMaps,
} from "~/renderer/common/visual/color-map";
import type {
  Field,
  FieldMap,
  FieldRank,
} from "~/renderer/common/visual/fields";
import type { GlyphScaleMode } from "~/renderer/common/visual/glyphs";
import type { ShadingMode } from "~/renderer/common/visual/particles";
import type { RenderMode } from "~/renderer/common/visual/particles-switch";
import { ExportButton } from "~/renderer/main/components/export";
import {
  backgroundColorNameAtom,
  cameraPositionAtom,
  cameraRotationAtom,
  colorFieldAtom,
  colorFieldModifierAtom,
  colorMapNameAtom,
  colorRangeAtom,
  colorRangeModeAtom,
  fieldAtom,
  fieldNameAtom,
  frameDataAtom,
  glyphScaleAtom,
  glyphScaleModeAtom,
  legendEnabledAtom,
  legendTickCountAtom,
  pointSizeAtom,
  projectionAtom,
  renderModeAtom,
  shadingModeAtom,
  toolModeAtom,
  userColorFieldNameAtom,
} from "~/renderer/main/state/viewport";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export function ViewControls() {
  return (
    <div className="flex h-8 shrink-0 items-center gap-1 px-2 py-1">
      {/* ---- Tool. ------------------------------------------------------- */}
      <ToolControls />

      <Separator orientation="vertical" />

      {/* ---- Camera. ----------------------------------------------------- */}
      <CameraControls />

      <Separator orientation="vertical" />

      {/* ---- Display. ---------------------------------------------------- */}
      <DisplayControls />

      <Separator orientation="vertical" />

      {/* ---- Render. ----------------------------------------------------- */}
      <RenderControls />

      <Separator orientation="vertical" />

      {/* ---- Export. ----------------------------------------------------- */}
      <ExportButton variant="ghost">
        <IconFileExport />
        Export…
      </ExportButton>
    </div>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// The recurring "label above a select" block.
interface LabeledSelectProps<Value extends string> {
  label: string;
  value: Value;
  onValueChange: (value: Value) => void;
  options: readonly SelectOption<Value>[];
}

function LabeledSelect<Value extends string>({
  label,
  value,
  onValueChange,
  options,
}: Readonly<LabeledSelectProps<Value>>) {
  return (
    <>
      <Text color="subtle">{label}</Text>
      <Select.Root
        value={value}
        onValueChange={onValueChange}
        options={options}
      >
        <Select.Trigger />
        <Select.Content />
      </Select.Root>
    </>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

function ToolControls() {
  const [toolMode, setToolMode] = useAtom(toolModeAtom);

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
            <div className="flex items-center gap-2">
              {icon}
              {label}
            </div>
            {toolMode === mode && <IconCheck className="ml-auto" />}
          </DropdownMenu.Item>
        ))}
      </DropdownMenu.Content>
    </DropdownMenu.Root>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

const projectionOptions: readonly SelectOption<Projection>[] = [
  { value: "perspective", label: "Perspective", icon: <IconPerspective /> },
  {
    value: "orthographic",
    label: "Orthographic",
    icon: <IconPerspectiveOff />,
  },
];

const backgroundColorOptions: readonly SelectOption<BackgroundColorName>[] =
  Object.entries(backgroundColors).map(([name, { label, color }]) => ({
    value: name as BackgroundColorName,
    label,
    icon: (
      <span
        className={cn("size-4 rounded-xs", color === null && surface())}
        style={
          color === null ? undefined : { backgroundColor: toCSSColor(color) }
        }
      />
    ),
  }));

function CameraControls() {
  const [projection, setProjection] = useAtom(projectionAtom);
  const [backgroundColorName, setBackgroundColorName] = useAtom(
    backgroundColorNameAtom,
  );
  const [cameraPosition, setCameraPosition] = useAtom(cameraPositionAtom);
  const [cameraRotation, setCameraRotation] = useAtom(cameraRotationAtom);

  return (
    <DropdownMenu.Root>
      <DropdownMenu.Trigger>
        <IconEye />
        View
      </DropdownMenu.Trigger>
      <DropdownMenu.Content className="w-90">
        <LabeledSelect
          label="Projection"
          value={projection}
          onValueChange={setProjection}
          options={projectionOptions}
        />

        <LabeledSelect
          label="Background Color"
          value={backgroundColorName}
          onValueChange={setBackgroundColorName}
          options={backgroundColorOptions}
        />

        <Section label="Advanced Camera" defaultOpen={false}>
          {/* ---- Position. ----------------------------------------------- */}
          <Text color="subtle">Position</Text>
          {(["x", "y", "z"] as const).map((axis) => (
            <NumberInput
              key={axis}
              slot={axis.toUpperCase()}
              value={cameraPosition[axis]}
              onValueChange={(value) => {
                if (value === null) return;
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
                if (value === null) return;
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

const renderModeOptions: readonly SelectOption<RenderMode>[] = [
  { value: "points", label: "Points", icon: <IconScalar /> },
  { value: "glyphs", label: "Glyphs", icon: <IconVector /> },
];

const glyphScaleModeOptions: readonly SelectOption<GlyphScaleMode>[] = [
  { value: "magnitude", label: "Magnitude", icon: <IconRulerMeasure /> },
  { value: "uniform", label: "Uniform", icon: <IconArrowsHorizontal /> },
];

const colorRangeModeOptions: readonly SelectOption<ColorRangeMode>[] = [
  { value: "auto", label: "Auto", icon: <IconAutoRange /> },
  { value: "manual", label: "Manual", icon: <IconArrowsHorizontal /> },
];

const colorMapOptions: readonly SelectOption<ColorMapName>[] = Object.entries(
  colorMaps,
).map(([name, { label }]) => ({
  value: name as ColorMapName,
  label,
  icon: <ColorBox name={name as ColorMapName} className="h-4 w-16" />,
}));

// Options for selecting a field from the frame data.
function fieldOptions(frameData: FieldMap): readonly SelectOption[] {
  return Array.from(frameData.entries()).map(([name, entry]) => ({
    value: name,
    label: <Mono>{name}</Mono>,
    icon: <FieldIcon rank={entry.rank} />,
  }));
}

// Options for selecting a color metric appropriate for the field rank.
function colorMetricOptions(
  rank: FieldRank,
): readonly SelectOption<"magnitude" | "determinant" | "component">[] {
  return [
    rank === 1
      ? { value: "magnitude", label: "Magnitude", icon: <IconRulerMeasure /> }
      : { value: "determinant", label: "Determinant", icon: <IconMatrix /> },
    { value: "component", label: "Component", icon: <IconComponent /> },
  ];
}

// Options for selecting a component of the field.
function componentOptions(field: Field): readonly SelectOption[] {
  const axes = ["x", "y", "z"].slice(0, field.dim);
  const names =
    field.rank === 1
      ? axes
      : axes.flatMap((row) => axes.map((col) => `${row}${col}`));
  return names.map((name, index) => ({
    value: index.toString(),
    label: <Mono>{name}</Mono>,
    icon: <IconComponent />,
  }));
}

function DisplayControls() {
  const frameData = useAtomValue(frameDataAtom);
  const field = useAtomValue(fieldAtom);
  const setFieldName = useSetAtom(fieldNameAtom);
  const [renderMode, setRenderMode] = useAtom(renderModeAtom);
  const [pointSize, setPointSize] = useAtom(pointSizeAtom);
  const [glyphScale, setGlyphScale] = useAtom(glyphScaleAtom);
  const [glyphScaleMode, setGlyphScaleMode] = useAtom(glyphScaleModeAtom);
  const colorField = useAtomValue(colorFieldAtom);
  const setUserColorFieldName = useSetAtom(userColorFieldNameAtom);
  const [colorFieldModifier, setColorFieldModifier] = useAtom(
    colorFieldModifierAtom,
  );
  const [colorMapName, setColorMapName] = useAtom(colorMapNameAtom);
  const [colorRangeMode, setColorRangeMode] = useAtom(colorRangeModeAtom);
  const [colorRange, setColorRange] = useAtom(colorRangeAtom);

  return (
    <DropdownMenu.Root>
      <DropdownMenu.Trigger>
        <IconDroplet />
        Display
      </DropdownMenu.Trigger>
      <DropdownMenu.Content className="w-90">
        <Section label="Display Mapping">
          <LabeledSelect
            label="Field"
            value={field.name}
            onValueChange={setFieldName}
            options={fieldOptions(frameData)}
          />

          {field.rank === 1 && (
            <LabeledSelect
              label="Representation"
              value={renderMode}
              onValueChange={setRenderMode}
              options={renderModeOptions}
            />
          )}

          {renderMode === "points" && (
            <>
              {/* ---- Point size. ----------------------------------------- */}
              <Text color="subtle">Point Size</Text>
              <NumberInput
                slot={<IconArrowsHorizontal />}
                min={0}
                value={pointSize}
                onValueChange={(value) => {
                  if (value === null) return;
                  setPointSize(value);
                }}
              />
            </>
          )}

          {renderMode === "glyphs" && (
            <>
              <LabeledSelect
                label="Glyph Scale Mode"
                value={glyphScaleMode}
                onValueChange={setGlyphScaleMode}
                options={glyphScaleModeOptions}
              />

              {/* ---- Glyph scale. ---------------------------------------- */}
              <Text color="subtle">Glyph Scale</Text>
              <NumberInput
                slot={<IconArrowsHorizontal />}
                min={0}
                value={glyphScale}
                onValueChange={(value) => {
                  if (value === null) return;
                  setGlyphScale(value);
                }}
              />
            </>
          )}
        </Section>

        <Section label="Coloring">
          {renderMode === "glyphs" && (
            <LabeledSelect
              label="Color By"
              value={colorField.name}
              onValueChange={setUserColorFieldName}
              options={fieldOptions(frameData)}
            />
          )}

          <LabeledSelect
            label="Color Range"
            value={colorRangeMode}
            onValueChange={setColorRangeMode}
            options={colorRangeModeOptions}
          />

          {colorRangeMode === "manual" && (
            <div className="flex items-center gap-1">
              {/* ---- Min / Max. ------------------------------------------ */}
              <NumberInput
                slot={<IconMathMin />}
                max={colorRange.max}
                value={colorRange.min}
                onValueChange={(value) => {
                  if (value === null) return;
                  setColorRange({ ...colorRange, min: value });
                }}
              />
              <Text color="subtle">-</Text>
              <NumberInput
                slot={<IconMathMax />}
                min={colorRange.min}
                value={colorRange.max}
                onValueChange={(value) => {
                  if (value === null) return;
                  setColorRange({ ...colorRange, max: value });
                }}
              />
            </div>
          )}

          <LabeledSelect
            label="Color Map"
            value={colorMapName}
            onValueChange={setColorMapName}
            options={colorMapOptions}
          />

          {colorField.rank > 0 && (
            <>
              <LabeledSelect
                label="Color Metric"
                value={
                  typeof colorFieldModifier === "number"
                    ? "component"
                    : colorFieldModifier
                }
                onValueChange={(value) => {
                  setColorFieldModifier(value === "component" ? 0 : value);
                }}
                options={colorMetricOptions(colorField.rank)}
              />

              {typeof colorFieldModifier === "number" && (
                <LabeledSelect
                  label="Component"
                  value={colorFieldModifier.toString()}
                  onValueChange={(value) => {
                    setColorFieldModifier(Math.trunc(Number(value)));
                  }}
                  options={componentOptions(colorField)}
                />
              )}
            </>
          )}
        </Section>
      </DropdownMenu.Content>
    </DropdownMenu.Root>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

const shadingModeOptions: readonly SelectOption<ShadingMode>[] = [
  { value: "flat", label: "Flat", icon: <IconSunOff /> },
  { value: "shaded", label: "Shaded", icon: <IconSun /> },
];

function RenderControls() {
  const [shadingMode, setShadingMode] = useAtom(shadingModeAtom);
  const [legendEnabled, setLegendEnabled] = useAtom(legendEnabledAtom);
  const [legendTickCount, setLegendTickCount] = useAtom(legendTickCountAtom);

  return (
    <DropdownMenu.Root>
      <DropdownMenu.Trigger>
        <IconSun />
        Render
      </DropdownMenu.Trigger>
      <DropdownMenu.Content className="w-90">
        <Section label="Shading">
          <LabeledSelect
            label="Shading Mode"
            value={shadingMode}
            onValueChange={setShadingMode}
            options={shadingModeOptions}
          />
        </Section>

        <Section label="Legend">
          {/* ---- Show legend. -------------------------------------------- */}
          <div className="flex items-center justify-between">
            <Text color="subtle">Show Legend</Text>
            <Switch
              checked={legendEnabled}
              onCheckedChange={setLegendEnabled}
            />
          </div>

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
                onValueChange={(value) => {
                  if (value === null) return;
                  setLegendTickCount(value);
                }}
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
