/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { Box, Flex } from "@radix-ui/themes";
import {
  type PointerEvent as ReactPointerEvent,
  useEffect,
  useRef,
  useState,
} from "react";
import { Euler, Vector2, Vector3 } from "three";

import { type MouseMode } from "~/components/mouse-mode";
import { chrome } from "~/components/classes";
import { type SelectionModifier, useSelection } from "~/components/selection";
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

type DragSelection =
  | {
      kind: "rect";
      start: Vector2;
      end: Vector2;
    }
  | {
      kind: "lasso";
      points: Vector2[];
    };

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export function Viewport() {
  const canvasRef = useRef<HTMLCanvasElement>(null);
  const [model] = useState(() => new ViewportModel());
  const [mouseMode, setMouseMode] = useState<MouseMode>("normal");
  const [dragSelection, setDragSelection] = useState<DragSelection | null>(
    null,
  );
  const [selectionModifier, setSelectionModifier] =
    useState<SelectionModifier>("replace");
  const { selectedParticleIndices, updateSelection } = useSelection();

  useEffect(() => {
    const canvas = canvasRef.current;
    if (canvas === null) return;

    const onContextMenu = (event: MouseEvent) => event.preventDefault();
    canvas.addEventListener("contextmenu", onContextMenu);

    const renderer = new Renderer(canvas);
    const detachRender = model.attachRenderer(renderer);

    const container = canvas.parentElement;
    assert(container !== null);
    renderer.resize(container.clientWidth, container.clientHeight);
    const resizeObserver = new ResizeObserver(() =>
      renderer.resize(container.clientWidth, container.clientHeight),
    );
    resizeObserver.observe(container, { box: "content-box" });

    return () => {
      detachRender();
      resizeObserver.disconnect();
      renderer.dispose();
      canvas.removeEventListener("contextmenu", onContextMenu);
    };
  }, [model]);

  const { frameData } = useStorage();

  useEffect(() => model.frameData.set(frameData), [model, frameData]);
  useEffect(() => model.setMouseMode(mouseMode), [model, mouseMode]);
  useEffect(
    () => model.setSelectedParticleIndices(selectedParticleIndices),
    [model, selectedParticleIndices],
  );
  useEffect(() => {
    const updateModifier = (event: KeyboardEvent) => {
      if (event.key === "Escape") {
        setDragSelection(null);
        setSelectionModifier("replace");
        setMouseMode("normal");
        return;
      }

      setSelectionModifier(selectionModifierFromKeys(event.altKey, event.shiftKey));
    };
    const resetModifier = () => setSelectionModifier("replace");

    window.addEventListener("keydown", updateModifier);
    window.addEventListener("keyup", updateModifier);
    window.addEventListener("blur", resetModifier);

    return () => {
      window.removeEventListener("keydown", updateModifier);
      window.removeEventListener("keyup", updateModifier);
      window.removeEventListener("blur", resetModifier);
    };
  }, [setMouseMode]);

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

  const onPointerDown = (event: ReactPointerEvent<HTMLDivElement>) => {
    const canvas = canvasRef.current;
    if (canvas === null) return;
    if (event.target !== canvas) return;
    setSelectionModifier(selectionModifierFromEvent(event));

    const rect = canvas.getBoundingClientRect();
    const start = new Vector2(
      event.clientX - rect.left,
      event.clientY - rect.top,
    );

    switch (mouseMode) {
      case "rect":
        setDragSelection({ kind: "rect", start, end: start.clone() });
        event.currentTarget.setPointerCapture(event.pointerId);
        break;
      case "lasso":
        setDragSelection({ kind: "lasso", points: [start] });
        event.currentTarget.setPointerCapture(event.pointerId);
        break;
      default:
        break;
    }
  };

  const onPointerMove = (event: ReactPointerEvent<HTMLDivElement>) => {
    setSelectionModifier(selectionModifierFromEvent(event));
    if (dragSelection === null) return;

    const canvas = canvasRef.current;
    if (canvas === null) return;
    const rect = canvas.getBoundingClientRect();
    const point = new Vector2(
      event.clientX - rect.left,
      event.clientY - rect.top,
    );

    switch (dragSelection.kind) {
      case "rect":
        setDragSelection({ ...dragSelection, end: point });
        break;
      case "lasso": {
        const lastPoint = dragSelection.points.at(-1);
        if (lastPoint !== undefined && lastPoint.distanceTo(point) < 4) return;
        setDragSelection({
          kind: "lasso",
          points: [...dragSelection.points, point],
        });
        break;
      }
    }
  };

  const finishDragSelection = (event: ReactPointerEvent<HTMLDivElement>) => {
    if (dragSelection === null) return;

    const modifier = selectionModifierFromEvent(event);
    setSelectionModifier(modifier);
    switch (dragSelection.kind) {
      case "rect":
        updateSelection(
          model.selectParticlesInRect(dragSelection.start, dragSelection.end),
          modifier,
        );
        break;
      case "lasso":
        updateSelection(
          model.selectParticlesInPolygon(dragSelection.points),
          modifier,
        );
        break;
    }

    if (event.currentTarget.hasPointerCapture(event.pointerId)) {
      event.currentTarget.releasePointerCapture(event.pointerId);
    }
    setDragSelection(null);
  };

  return (
    <Flex direction="column" width="100%" height="100%" gap="1px">
      <ViewControls
        frameData={frameData}
        field={field}
        setFieldByName={(value) => model.fieldName.set(value)}
        mouseMode={mouseMode}
        setMouseMode={setMouseMode}
        projection={projection}
        setProjection={(value) => model.projection.set(value)}
        backgroundColorName={backgroundColorName}
        setBackgroundColorName={(value) => model.backgroundColorName.set(value)}
        renderMode={renderMode}
        setRenderMode={(value) => model.renderMode.set(value)}
        colorField={colorField}
        {...(renderMode === "glyphs" && {
          setColorFieldByName: (value: string) =>
            model.userColorFieldName.set(value),
        })}
        colorFieldModifier={colorFieldModifier}
        setColorFieldModifier={(value) => model.colorFieldModifier.set(value)}
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

      <Box
        flexGrow="1"
        width="100%"
        height="100%"
        overflow="hidden"
        position="relative"
        className={chrome({ direction: "bl" })}
        style={{ cursor: viewportCursor(mouseMode, selectionModifier) }}
        onPointerDown={onPointerDown}
        onPointerMove={onPointerMove}
        onPointerUp={finishDragSelection}
        onPointerCancel={finishDragSelection}
      >
        <canvas ref={canvasRef} style={{ position: "absolute" }} />
        <SelectionOverlay selection={dragSelection} />

        <ViewHUD
          appearance={backgroundColors[backgroundColorName].appearance}
          cameraRotation={cameraRotation}
          setCameraRotation={(value) => model.cameraRotation.set(value)}
          colorMapName={colorMapName}
          colorRange={colorRange}
          colorTitle={colorTitle}
          mouseMode={mouseMode}
          selectionModifier={selectionModifier}
        />
      </Box>
    </Flex>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

function SelectionOverlay({
  selection,
}: Readonly<{ selection: DragSelection | null }>) {
  if (selection === null) return null;

  return (
    <svg
      width="100%"
      height="100%"
      style={{ position: "absolute", inset: 0, pointerEvents: "none" }}
    >
      {selection.kind === "rect" && (
        <rect
          x={Math.min(selection.start.x, selection.end.x)}
          y={Math.min(selection.start.y, selection.end.y)}
          width={Math.abs(selection.end.x - selection.start.x)}
          height={Math.abs(selection.end.y - selection.start.y)}
          fill="rgba(59, 130, 246, 0.15)"
          stroke="rgb(59, 130, 246)"
          strokeWidth="1.5"
          strokeDasharray="6 4"
        />
      )}

      {selection.kind === "lasso" && (
        <polyline
          points={selection.points.map(({ x, y }) => `${x},${y}`).join(" ")}
          fill="rgba(59, 130, 246, 0.15)"
          stroke="rgb(59, 130, 246)"
          strokeWidth="1.5"
          strokeDasharray="6 4"
          strokeLinejoin="round"
        />
      )}
    </svg>
  );
}

function selectionModifierFromEvent(
  event: ReactPointerEvent<HTMLDivElement>,
): SelectionModifier {
  return selectionModifierFromKeys(event.altKey, event.shiftKey);
}

function selectionModifierFromKeys(
  altKey: boolean,
  shiftKey: boolean,
): SelectionModifier {
  if (altKey) return "subtract";
  if (shiftKey) return "add";
  return "replace";
}

function viewportCursor(
  mouseMode: MouseMode,
  selectionModifier: SelectionModifier,
) {
  if (mouseMode === "normal") return "default";

  switch (selectionModifier) {
    case "replace":
      return "crosshair";
    case "add":
      return "copy";
    case "subtract":
      return "not-allowed";
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class ViewportModel {
  private renderer: Renderer | null = null;
  private mouseMode: MouseMode = "normal";
  private selectedParticleIndices: number[] = [];

  public readonly projection = signal<Projection>("orthographic");
  public readonly backgroundColorName = signal<BackgroundColorName>("none");
  public readonly cameraPosition = signal(new Vector3());
  public readonly cameraRotation = signal(new Euler());

  public readonly frameData = signal(new FieldMap({}));

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

  public attachRenderer(renderer: Renderer) {
    assert(this.renderer === null);

    this.renderer = renderer;

    renderer.setProjection(this.projection.get());
    renderer.setBackgroundColor(
      backgroundColors[this.backgroundColorName.get()],
    );
    renderer.cameraController.position.copy(this.cameraPosition.get());
    renderer.cameraController.rotation.copy(this.cameraRotation.get());
    renderer.cameraController.enabled = this.mouseMode === "normal";
    renderer.setSelectedParticleIndices(this.selectedParticleIndices);
    this.pushFieldsAndColoring();

    const handleChange = () => this.pullCameraState();
    const controller = renderer.cameraController;
    controller.addEventListener("changed", handleChange);

    return () => {
      controller.removeEventListener("changed", handleChange);
      this.renderer = null;
    };
  }

  public setMouseMode(mode: MouseMode) {
    this.mouseMode = mode;
    if (this.renderer !== null) {
      this.renderer.cameraController.enabled = mode === "normal";
    }
  }

  public setSelectedParticleIndices(indices: number[]) {
    this.selectedParticleIndices = indices;
    this.renderer?.setSelectedParticleIndices(indices);
  }

  public selectParticlesInRect(start: Vector2, end: Vector2) {
    return this.renderer?.selectParticlesInRect(start, end) ?? [];
  }

  public selectParticlesInPolygon(points: Vector2[]) {
    return this.renderer?.selectParticlesInPolygon(points) ?? [];
  }

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

    if (renderMode === "points") renderer.setPointSize(this.pointSize.get());
    if (renderMode === "glyphs") {
      renderer.setGlyphScale(this.glyphScale.get());
      renderer.setGlyphScaleMode(this.glyphScaleMode.get());
    }

    this.pushColorRange();
    this.pushColorMap();
  }

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
