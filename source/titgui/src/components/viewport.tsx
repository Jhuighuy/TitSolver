/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { Box, Flex } from "@radix-ui/themes";
import { useEffect, useRef, useState } from "react";
import { Euler, Vector3 } from "three";

import { chrome } from "~/components/classes";
import { useStorage } from "~/components/storage";
import { ViewControls } from "~/components/view-controls";
import { ViewHUD } from "~/components/view-hud";
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
  fieldModifierDefault,
  fieldModifierToString,
  isValidFieldModifier,
} from "~/visual/fields";
import type { GlyphScaleMode } from "~/visual/glyphs";
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
  const cameraRotation = useSignalValue(model.cameraRotation);
  const field = useSignalValue(model.field);
  const renderMode = useSignalValue(model.renderMode);
  const colorField = useSignalValue(model.colorField);
  const colorFieldModifier = useSignalValue(model.colorFieldModifier);
  const colorTitle = useSignalValue(model.colorTitle);
  const pointSize = useSignalValue(model.pointSize);
  const glyphScale = useSignalValue(model.glyphScale);
  const glyphScaleMode = useSignalValue(model.glyphScaleMode);
  const colorMapName = useSignalValue(model.colorMapName);
  const colorRange = useSignalValue(model.colorRange);
  const colorRangeMode = useSignalValue(model.colorRangeMode);

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
        setColorRange={(value) => model.userColorRange.set(value)}
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

  public readonly pointSize = signal(25.0);

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
  public readonly userColorRange = scoped(
    () => colorRangeDefault,
    [
      this.fieldName,
      this.renderMode,
      this.colorFieldName,
      this.colorFieldModifier,
    ],
  );
  public readonly colorRange = derived(() => {
    return this.colorRangeMode.get() === "auto"
      ? this.autoColorRange.get()
      : this.userColorRange.get();
  }, [this.colorRangeMode, this.autoColorRange, this.userColorRange]);

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
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
