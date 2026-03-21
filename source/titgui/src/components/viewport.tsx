/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { Box, Flex } from "@radix-ui/themes";
import { useEffect, useRef, useState } from "react";
import { Euler, Vector3 } from "three";
import { z } from "zod";

import { chrome } from "~/components/classes";
import { useStorage } from "~/components/storage";
import { ViewControls } from "~/components/view-controls";
import { ViewHUD } from "~/components/view-hud";
import { usePersistedState } from "~/hooks/use-persisted-state";
import { useSignalValue } from "~/hooks/use-signal";
import { derived, scoped, signal } from "~/signals";
import { assert } from "~/utils";
import {
  type BackgroundColorName,
  backgroundColors,
} from "~/visual/background-color";
import type { Projection } from "~/visual/camera";
import {
  type ColorMapName,
  type ColorRangeMode,
  colorMaps,
  colorRangeDefault,
} from "~/visual/color-map";
import {
  FieldMap,
  type FieldModifier,
  fieldModifierDefault,
  fieldModifierToString,
  isValidFieldModifier,
} from "~/visual/fields";
import type { GlyphScaleMode } from "~/visual/glyphs";
import type { ShadingMode } from "~/visual/particles";
import { isValidRenderMode, type RenderMode } from "~/visual/particles-switch";
import { Renderer } from "~/visual/renderer";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export function Viewport() {
  // ---- Renderer. ------------------------------------------------------------

  const canvasRef = useRef<HTMLCanvasElement>(null);
  const [model] = useState(() => new ViewportModel());

  useEffect(() => {
    // Get canvas.
    const canvas = canvasRef.current;
    if (canvas === null) return;

    // Prevent context menu for right mouse drag.
    const onContextMenu = (event: MouseEvent) => event.preventDefault();
    canvas.addEventListener("contextmenu", onContextMenu);

    // Create renderer and attach to model.
    const renderer = new Renderer(canvas);
    const detachRender = model.attachRenderer(renderer);

    // Keep renderer size synchronized with container size.
    const container = canvas.parentElement;
    assert(container !== null);
    renderer.resize(container.clientWidth, container.clientHeight);
    const resizeObserver = new ResizeObserver(() =>
      renderer.resize(container.clientWidth, container.clientHeight),
    );
    resizeObserver.observe(container, { box: "content-box" });

    return () => {
      // Dispose renderer and listeners.
      detachRender();
      resizeObserver.disconnect();
      renderer.dispose();
      canvas.removeEventListener("contextmenu", onContextMenu);
    };
  }, [model]);

  // ---- Frame data. ----------------------------------------------------------

  const { frameData } = useStorage();

  useEffect(() => model.frameData.set(frameData), [model, frameData]);

  // ---- State. ---------------------------------------------------------------

  const projection = useSignalValue(model.projection);
  const backgroundColorName = useSignalValue(model.backgroundColorName);
  const cameraPosition = useSignalValue(model.cameraPosition);
  const cameraRotation = useSignalValue(model.cameraRotation);
  const field = useSignalValue(model.field);
  const renderMode = useSignalValue(model.renderMode);
  const colorField = useSignalValue(model.colorField);
  const colorFieldModifier = useSignalValue(model.colorFieldModifier);
  const colorTitle = useSignalValue(model.colorTitle);
  const shadingMode = useSignalValue(model.shadingMode);
  const pointSize = useSignalValue(model.pointSize);
  const glyphScale = useSignalValue(model.glyphScale);
  const glyphScaleMode = useSignalValue(model.glyphScaleMode);
  const colorMapName = useSignalValue(model.colorMapName);
  const colorRange = useSignalValue(model.colorRange);
  const colorRangeMode = useSignalValue(model.colorRangeMode);

  // ---- Presets. -------------------------------------------------------------

  const [presets, setPresets] = usePersistedState(
    viewportPresetsKey,
    viewportPresetsSchema,
    [],
  );

  const presetNames = presets.map(({ name }) => name);

  function savePreset(name: string) {
    const trimmedName = name.trim();
    if (trimmedName.length === 0) return;

    const preset = { name: trimmedName, state: model.exportPreset() };
    setPresets((prev) =>
      [...prev.filter(({ name }) => name !== trimmedName), preset].sort(
        (left, right) => left.name.localeCompare(right.name),
      ),
    );
  }

  function restorePreset(name: string) {
    const preset = presets.find((candidate) => candidate.name === name);
    if (preset !== undefined) model.applyPreset(preset.state);
  }

  function deletePreset(name: string) {
    setPresets((prev) => prev.filter((preset) => preset.name !== name));
  }

  // ---- Layout. --------------------------------------------------------------

  return (
    <Flex direction="column" width="100%" height="100%" gap="1px">
      {/* ---- Controls. --------------------------------------------------- */}
      <ViewControls
        frameData={frameData}
        /* Camera. */
        projection={projection}
        setProjection={(value) => model.projection.set(value)}
        backgroundColorName={backgroundColorName}
        setBackgroundColorName={(value) => model.backgroundColorName.set(value)}
        cameraPosition={cameraPosition}
        setCameraPosition={(value) => model.cameraPosition.set(value)}
        cameraRotation={cameraRotation}
        setCameraRotation={(value) => model.cameraRotation.set(value)}
        /* Field selection. */
        field={field}
        setFieldByName={(value) => model.fieldName.set(value)}
        renderMode={renderMode}
        setRenderMode={(value) => model.renderMode.set(value)}
        colorField={colorField}
        {...(renderMode === "glyphs" && {
          setColorFieldByName: (value: string) =>
            model.userColorFieldName.set(value),
        })}
        colorFieldModifier={colorFieldModifier}
        setColorFieldModifier={(value) => model.colorFieldModifier.set(value)}
        /* Render parameters. */
        shadingMode={shadingMode}
        setShadingMode={(value) => model.shadingMode.set(value)}
        pointSize={pointSize}
        setPointSize={(value) => model.pointSize.set(value)}
        glyphScale={glyphScale}
        setGlyphScale={(value) => model.glyphScale.set(value)}
        glyphScaleMode={glyphScaleMode}
        setGlyphScaleMode={(value) => model.glyphScaleMode.set(value)}
        colorMapName={colorMapName}
        setColorMapName={(value) => model.colorMapName.set(value)}
        colorRangeMode={colorRangeMode}
        setColorRangeMode={(value) => model.colorRangeMode.set(value)}
        colorRange={colorRange}
        setColorRange={(value) => model.colorRange.set(value)}
        /* Presets. */
        presetNames={presetNames}
        savePreset={savePreset}
        restorePreset={restorePreset}
        deletePreset={deletePreset}
      />

      {/* ---- Canvas. ----------------------------------------------------- */}
      <Box
        flexGrow="1"
        width="100%"
        height="100%"
        overflow="hidden"
        position="relative"
        className={chrome({ direction: "bl" })}
      >
        <canvas ref={canvasRef} style={{ position: "absolute" }} />

        <ViewHUD
          appearance={backgroundColors[backgroundColorName].appearance}
          cameraRotation={cameraRotation}
          setCameraRotation={(value) => model.cameraRotation.set(value)}
          colorMapName={colorMapName}
          colorRange={colorRange}
          colorTitle={colorTitle}
        />
      </Box>
    </Flex>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class ViewportModel {
  private renderer: Renderer | null = null;

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  public readonly projection = signal<Projection>("orthographic");
  public readonly backgroundColorName = signal<BackgroundColorName>("none");
  public readonly cameraPosition = signal(new Vector3());
  public readonly cameraRotation = signal(new Euler());

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  public readonly frameData = signal(new FieldMap({}));

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  public readonly fieldName = signal("rho");
  public readonly field = derived(
    () => this.frameData.get().get(this.fieldName.get()),
    [this.frameData, this.fieldName],
  );

  public readonly renderMode = scoped<RenderMode>(
    () => (this.field.get().rank === 1 ? "glyphs" : "points"),
    [this.fieldName],
  );

  public readonly userColorFieldName = scoped(
    () => this.fieldName.get(),
    [this.fieldName],
  );
  public readonly colorFieldName = derived(() => {
    return this.renderMode.get() === "glyphs"
      ? this.userColorFieldName.get()
      : this.fieldName.get();
  }, [this.fieldName, this.renderMode, this.userColorFieldName]);
  public readonly colorField = derived(
    () => this.frameData.get().get(this.colorFieldName.get()),
    [this.frameData, this.field, this.renderMode, this.colorFieldName],
  );

  public readonly colorFieldModifier = scoped(
    () => fieldModifierDefault(this.colorField.get()),
    [this.fieldName, this.renderMode, this.colorFieldName],
  );

  public readonly colorTitle = derived(() => {
    return fieldModifierToString(
      this.colorField.get(),
      this.colorFieldModifier.get(),
    );
  }, [this.colorField, this.colorFieldModifier]);

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  public readonly shadingMode = signal<ShadingMode>("shaded");

  public readonly pointSize = signal(10.0);

  public readonly glyphScale = scoped(() => 0.02, [this.fieldName]);
  public readonly glyphScaleMode = scoped<GlyphScaleMode>(
    () => "uniform",
    [this.fieldName],
  );

  public readonly colorMapName = scoped<ColorMapName>(
    () => "jet",
    [
      this.fieldName,
      this.renderMode,
      this.colorFieldName,
      this.colorFieldModifier,
    ],
  );

  public readonly colorRangeMode = scoped<ColorRangeMode>(
    () => "auto",
    [
      this.fieldName,
      this.renderMode,
      this.colorFieldName,
      this.colorFieldModifier,
    ],
  );

  private readonly autoColorRange = signal(colorRangeDefault);
  public readonly colorRange = scoped(
    () => colorRangeDefault,
    [
      this.fieldName,
      this.renderMode,
      this.colorFieldName,
      this.colorFieldModifier,
    ],
  );

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  public constructor() {
    this.projection.subscribe(() =>
      this.renderer?.setProjection(this.projection.get()),
    );
    this.backgroundColorName.subscribe(() =>
      this.renderer?.setBackgroundColor(
        backgroundColors[this.backgroundColorName.get()],
      ),
    );
    this.cameraPosition.subscribe(() =>
      this.renderer?.cameraController.position.copy(this.cameraPosition.get()),
    );
    this.cameraRotation.subscribe(() =>
      this.renderer?.cameraController.rotation.copy(this.cameraRotation.get()),
    );

    this.field.subscribe(() => this.pushFieldsAndColoring());
    this.renderMode.subscribe(() => this.pushFieldsAndColoring());
    this.colorField.subscribe(() => this.pushFieldsAndColoring());
    this.colorFieldModifier.subscribe(() => this.pushFieldsAndColoring());

    this.shadingMode.subscribe(() =>
      this.renderer?.setShadingMode(this.shadingMode.get()),
    );
    this.pointSize.subscribe(() =>
      this.renderer?.setPointSize(this.pointSize.get()),
    );
    this.glyphScale.subscribe(() =>
      this.renderer?.setGlyphScale(this.glyphScale.get()),
    );
    this.glyphScaleMode.subscribe(() =>
      this.renderer?.setGlyphScaleMode(this.glyphScaleMode.get()),
    );

    this.colorMapName.subscribe(() => this.pushColorMap());
    this.colorRangeMode.subscribe(() => this.pushColorRange());
    this.colorRange.subscribe(() => this.pushColorRange());
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  public attachRenderer(renderer: Renderer) {
    assert(this.renderer === null);

    this.renderer = renderer;

    // Initialize renderer state.
    renderer.setProjection(this.projection.get());
    renderer.setBackgroundColor(
      backgroundColors[this.backgroundColorName.get()],
    );
    renderer.cameraController.position.copy(this.cameraPosition.get());
    renderer.cameraController.rotation.copy(this.cameraRotation.get());
    this.pushFieldsAndColoring();

    // Update camera state on change.
    const handleChange = () => this.pullCameraState();
    const controller = renderer.cameraController;
    controller.addEventListener("changed", handleChange);

    return () => {
      controller.removeEventListener("changed", handleChange);
      this.renderer = null;
    };
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  private pullCameraState() {
    const renderer = this.renderer;
    if (renderer === null) return;

    this.cameraPosition.set(
      new Vector3().copy(renderer.cameraController.position),
    );
    this.cameraRotation.set(
      new Euler().copy(renderer.cameraController.rotation),
    );
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  private pushFieldsAndColoring() {
    const renderer = this.renderer;
    if (renderer === null) return;

    const field = this.field.get();
    const renderMode = this.renderMode.get();
    assert(isValidRenderMode(field, renderMode));
    renderer.setRenderMode(renderMode);

    const colorField = this.colorField.get();
    const colorFieldModifier = this.colorFieldModifier.get();
    assert(isValidFieldModifier(colorField, colorFieldModifier));
    this.autoColorRange.set(
      renderer.setRenderData(
        this.frameData.get(),
        field,
        colorField,
        colorFieldModifier,
      ),
    );

    renderer.setShadingMode(this.shadingMode.get());

    if (renderMode === "points") {
      renderer.setPointSize(this.pointSize.get());
    }
    if (renderMode === "glyphs") {
      renderer.setGlyphScale(this.glyphScale.get());
      renderer.setGlyphScaleMode(this.glyphScaleMode.get());
    }

    this.pushColorRange();
    this.pushColorMap();
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  private pushColorRange() {
    if (this.colorRangeMode.get() === "auto") {
      this.colorRange.set(this.autoColorRange.get());
    }

    this.renderer?.setRenderColorRange(
      this.renderMode.get(),
      this.colorRange.get(),
    );
  }

  private pushColorMap() {
    this.renderer?.setRenderColorMap(
      this.renderMode.get(),
      colorMaps[this.colorMapName.get()],
    );
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  public exportPreset(): ViewportPresetState {
    const cameraPosition = this.cameraPosition.get();
    const cameraRotation = this.cameraRotation.get();
    return {
      projection: this.projection.get(),
      backgroundColorName: this.backgroundColorName.get(),
      cameraPosition: [cameraPosition.x, cameraPosition.y, cameraPosition.z],
      cameraRotation: [cameraRotation.x, cameraRotation.y, cameraRotation.z],
      fieldName: this.fieldName.get(),
      renderMode: this.renderMode.get(),
      userColorFieldName: this.userColorFieldName.get(),
      colorFieldModifier: this.colorFieldModifier.get(),
      pointSize: this.pointSize.get(),
      shadingMode: this.shadingMode.get(),
      glyphScale: this.glyphScale.get(),
      glyphScaleMode: this.glyphScaleMode.get(),
      colorMapName: this.colorMapName.get(),
      colorRangeMode: this.colorRangeMode.get(),
      colorRange: this.colorRange.get(),
    };
  }

  public applyPreset(state: ViewportPresetState) {
    const frameData = this.frameData.get();

    if (frameData.has(state.fieldName)) this.fieldName.set(state.fieldName);
    if (frameData.has(state.userColorFieldName)) {
      this.userColorFieldName.set(state.userColorFieldName);
    }

    const field = this.field.get();
    if (isValidRenderMode(field, state.renderMode)) {
      this.renderMode.set(state.renderMode);
    }

    const colorFieldName = this.colorFieldName.get();
    const colorField = frameData.get(colorFieldName);
    if (isValidFieldModifier(colorField, state.colorFieldModifier)) {
      this.colorFieldModifier.set(state.colorFieldModifier);
    }

    this.projection.set(state.projection);
    this.backgroundColorName.set(state.backgroundColorName);
    this.cameraPosition.set(new Vector3(...state.cameraPosition));
    this.cameraRotation.set(new Euler(...state.cameraRotation));
    this.pointSize.set(state.pointSize);
    this.shadingMode.set(state.shadingMode);
    this.glyphScale.set(state.glyphScale);
    this.glyphScaleMode.set(state.glyphScaleMode);
    this.colorMapName.set(state.colorMapName);
    this.colorRangeMode.set(state.colorRangeMode);
    this.colorRange.set(state.colorRange);
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

type ViewportPresetState = {
  projection: Projection;
  backgroundColorName: BackgroundColorName;
  cameraPosition: [number, number, number];
  cameraRotation: [number, number, number];
  fieldName: string;
  renderMode: RenderMode;
  userColorFieldName: string;
  colorFieldModifier: FieldModifier;
  pointSize: number;
  shadingMode: ShadingMode;
  glyphScale: number;
  glyphScaleMode: GlyphScaleMode;
  colorMapName: ColorMapName;
  colorRangeMode: ColorRangeMode;
  colorRange: { min: number; max: number };
};

const viewportPresetStateSchema: z.ZodType<ViewportPresetState> = z.object({
  projection: z.union([z.literal("orthographic"), z.literal("perspective")]),
  backgroundColorName: z.custom<BackgroundColorName>(
    (value) => typeof value === "string" && value in backgroundColors,
  ),
  cameraPosition: z.tuple([z.number(), z.number(), z.number()]),
  cameraRotation: z.tuple([z.number(), z.number(), z.number()]),
  fieldName: z.string().min(1),
  renderMode: z.union([z.literal("points"), z.literal("glyphs")]),
  userColorFieldName: z.string().min(1),
  colorFieldModifier: z.union([
    z.literal("magnitude"),
    z.literal("determinant"),
    z.int().nonnegative(),
  ]),
  pointSize: z.number().positive(),
  shadingMode: z.union([z.literal("flat"), z.literal("shaded")]),
  glyphScale: z.number().positive(),
  glyphScaleMode: z.union([z.literal("magnitude"), z.literal("uniform")]),
  colorMapName: z.custom<ColorMapName>(
    (value) => typeof value === "string" && value in colorMaps,
  ),
  colorRangeMode: z.union([z.literal("auto"), z.literal("manual")]),
  colorRange: z.object({
    min: z.number(),
    max: z.number(),
  }),
});

const viewportPresetsSchema: z.ZodType<
  Array<{ name: string; state: ViewportPresetState }>
> = z.array(
  z.object({
    name: z.string().min(1),
    state: viewportPresetStateSchema,
  }),
);

const viewportPresetsKey = "viewport:presets";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
