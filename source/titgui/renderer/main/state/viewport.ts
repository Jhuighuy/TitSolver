/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { atom, getDefaultStore } from "jotai";
import { Euler, Vector3 } from "three";

import { scopedAtom } from "~/renderer/common/atoms";
import {
  type BackgroundColorName,
  backgroundColors,
} from "~/renderer/common/visual/background-color";
import type { Projection } from "~/renderer/common/visual/camera";
import {
  type ColorMapName,
  type ColorRange,
  type ColorRangeMode,
  colorMaps,
  colorRangeDefault,
} from "~/renderer/common/visual/color-map";
import {
  FieldMap,
  fieldModifierDefault,
  fieldModifierToString,
  isValidFieldModifier,
} from "~/renderer/common/visual/fields";
import type { GlyphScaleMode } from "~/renderer/common/visual/glyphs";
import type { ShadingMode } from "~/renderer/common/visual/particles";
import {
  isValidRenderMode,
  type RenderMode,
} from "~/renderer/common/visual/particles-switch";
import type { Renderer } from "~/renderer/common/visual/renderer";
import type { SelectionCommand } from "~/renderer/common/visual/selection";
import { assert } from "~/shared/utils";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
 * Viewport interaction tool.
 */
export type ToolMode = "normal" | "rect" | "lasso";

export const toolModeAtom = atom<ToolMode>("normal");

/** Pending selection command; consumed by the renderer binding. */
export const selectionCommandAtom = atom<SelectionCommand | null>(null);

export const selectionCountAtom = atom(0);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export const projectionAtom = atom<Projection>("orthographic");
export const backgroundColorNameAtom = atom<BackgroundColorName>("default");
export const cameraPositionAtom = atom(new Vector3());
export const cameraRotationAtom = atom(new Euler());

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/** Fields of the currently displayed frame; written by the storage state. */
export const frameDataAtom = atom(new FieldMap({}));

/** Field selected by the user. May be absent from the current frame data. */
export const fieldNameAtom = atom("rho");

// Selected field name, or the always-present density field when the
// selection is absent from the current frame data (e.g. right after a
// storage refresh). The user selection is kept, so the view returns to it
// once the field is available again.
const effectiveFieldNameAtom = atom((get) => {
  const fieldName = get(fieldNameAtom);
  return get(frameDataAtom).has(fieldName) ? fieldName : "rho";
});

export const fieldAtom = atom((get) =>
  get(frameDataAtom).get(get(effectiveFieldNameAtom)),
);

// Scope for view settings that are remembered per displayed field.
const fieldScopeAtom = atom((get) => get(effectiveFieldNameAtom));

export const renderModeAtom = scopedAtom<RenderMode>(fieldScopeAtom, (get) =>
  get(fieldAtom).rank === 1 ? "glyphs" : "points",
);

export const pointSizeAtom = atom(10);

export const glyphScaleModeAtom = scopedAtom<GlyphScaleMode>(
  fieldScopeAtom,
  () => "uniform",
);

const uniformGlyphScaleAtom = atom(0.02);
const magnitudeGlyphScaleAtom = scopedAtom(fieldScopeAtom, () => 0.02);

/** Glyph scale for the active glyph scale mode. */
export const glyphScaleAtom = atom(
  (get) =>
    get(glyphScaleModeAtom) === "uniform"
      ? get(uniformGlyphScaleAtom)
      : get(magnitudeGlyphScaleAtom),
  (get, set, value: number) => {
    if (get(glyphScaleModeAtom) === "uniform") {
      set(uniformGlyphScaleAtom, value);
    } else {
      set(magnitudeGlyphScaleAtom, value);
    }
  },
);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/** Color field chosen by the user; only relevant in glyph mode. */
export const userColorFieldNameAtom = scopedAtom(fieldScopeAtom, (get) =>
  get(effectiveFieldNameAtom),
);

export const colorFieldNameAtom = atom((get) => {
  const colorFieldName =
    get(renderModeAtom) === "glyphs"
      ? get(userColorFieldNameAtom)
      : get(effectiveFieldNameAtom);
  return get(frameDataAtom).has(colorFieldName)
    ? colorFieldName
    : get(effectiveFieldNameAtom);
});

export const colorFieldAtom = atom((get) =>
  get(frameDataAtom).get(get(colorFieldNameAtom)),
);

// Scope for coloring settings: per field, representation, and color field.
const coloringScopeAtom = atom((get) =>
  [
    get(effectiveFieldNameAtom),
    get(renderModeAtom),
    get(colorFieldNameAtom),
  ].join(":"),
);

export const colorFieldModifierAtom = scopedAtom(coloringScopeAtom, (get) =>
  fieldModifierDefault(get(colorFieldAtom)),
);

export const colorTitleAtom = atom((get) =>
  fieldModifierToString(get(colorFieldAtom), get(colorFieldModifierAtom)),
);

// Scope for color mapping settings: additionally per color metric.
const colorMappingScopeAtom = atom((get) =>
  [get(coloringScopeAtom), String(get(colorFieldModifierAtom))].join(":"),
);

export const colorMapNameAtom = scopedAtom<ColorMapName>(
  colorMappingScopeAtom,
  () => "jet",
);

export const colorRangeModeAtom = scopedAtom<ColorRangeMode>(
  colorMappingScopeAtom,
  () => "auto",
);

/** Range computed from the data; written by the renderer binding. */
const autoColorRangeAtom = atom<ColorRange>(colorRangeDefault);

// Manually entered range. Until first edited in a scope, it mirrors the
// automatic range, so switching to manual mode starts from sensible values.
const manualColorRangeAtom = scopedAtom(colorMappingScopeAtom, (get) =>
  get(autoColorRangeAtom),
);

/** Effective color range: automatic or manually entered. */
export const colorRangeAtom = atom(
  (get) =>
    get(colorRangeModeAtom) === "auto"
      ? get(autoColorRangeAtom)
      : get(manualColorRangeAtom),
  (_get, set, value: ColorRange) => {
    set(manualColorRangeAtom, value);
  },
);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export const shadingModeAtom = atom<ShadingMode>("shaded");
export const legendEnabledAtom = atom(true);
export const legendTickCountAtom = atom(5);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
 * Bind a renderer to the viewport state: push the current state, keep the
 * renderer synchronized with atom changes, and pull camera movements back
 * into the atoms. Returns an unbind function.
 */
export function bindViewportRenderer(renderer: Renderer) {
  const store = getDefaultStore();
  const unsubscribers: (() => void)[] = [];
  const sub = (
    anAtom: Parameters<typeof store.sub>[0],
    listener: () => void,
  ) => {
    unsubscribers.push(store.sub(anAtom, listener));
  };

  // ---- Push helpers. ---------------------------------------------------------

  const pushColorRange = () => {
    renderer.setRenderColorRange(
      store.get(renderModeAtom),
      store.get(colorRangeAtom),
    );
  };

  const pushColorMap = () => {
    renderer.setRenderColorMap(
      store.get(renderModeAtom),
      colorMaps[store.get(colorMapNameAtom)],
    );
  };

  const pushFieldsAndColoring = () => {
    const field = store.get(fieldAtom);
    const renderMode = store.get(renderModeAtom);
    assert(isValidRenderMode(field, renderMode));
    renderer.setRenderMode(renderMode);

    const colorField = store.get(colorFieldAtom);
    const colorFieldModifier = store.get(colorFieldModifierAtom);
    assert(isValidFieldModifier(colorField, colorFieldModifier));
    store.set(
      autoColorRangeAtom,
      renderer.setRenderData(
        store.get(frameDataAtom),
        field,
        colorField,
        colorFieldModifier,
      ),
    );

    renderer.setShadingMode(store.get(shadingModeAtom));

    if (renderMode === "points") {
      renderer.setPointSize(store.get(pointSizeAtom));
    }
    if (renderMode === "glyphs") {
      renderer.setGlyphScale(store.get(glyphScaleAtom));
      renderer.setGlyphScaleMode(store.get(glyphScaleModeAtom));
    }

    pushColorRange();
    pushColorMap();
  };

  // ---- Initial push. ----------------------------------------------------------

  renderer.setProjection(store.get(projectionAtom));
  renderer.setBackgroundColor(
    backgroundColors[store.get(backgroundColorNameAtom)],
  );
  renderer.cameraController.position.copy(store.get(cameraPositionAtom));
  renderer.cameraController.rotation.copy(store.get(cameraRotationAtom));
  renderer.cameraController.camera.updateProjectionMatrix();
  pushFieldsAndColoring();

  // ---- Atoms to renderer. -------------------------------------------------------

  sub(projectionAtom, () => {
    renderer.setProjection(store.get(projectionAtom));
  });
  sub(backgroundColorNameAtom, () => {
    renderer.setBackgroundColor(
      backgroundColors[store.get(backgroundColorNameAtom)],
    );
  });
  sub(cameraPositionAtom, () => {
    renderer.cameraController.position.copy(store.get(cameraPositionAtom));
  });
  sub(cameraRotationAtom, () => {
    renderer.cameraController.rotation.copy(store.get(cameraRotationAtom));
  });

  sub(fieldAtom, pushFieldsAndColoring);
  sub(renderModeAtom, pushFieldsAndColoring);
  sub(colorFieldAtom, pushFieldsAndColoring);
  sub(colorFieldModifierAtom, pushFieldsAndColoring);

  sub(pointSizeAtom, () => {
    renderer.setPointSize(store.get(pointSizeAtom));
  });
  sub(glyphScaleAtom, () => {
    renderer.setGlyphScale(store.get(glyphScaleAtom));
  });
  sub(glyphScaleModeAtom, () => {
    renderer.setGlyphScaleMode(store.get(glyphScaleModeAtom));
  });

  sub(colorMapNameAtom, pushColorMap);
  sub(colorRangeAtom, pushColorRange);

  sub(shadingModeAtom, () => {
    renderer.setShadingMode(store.get(shadingModeAtom));
  });

  // ---- Selection commands. ------------------------------------------------------

  sub(selectionCommandAtom, () => {
    const command = store.get(selectionCommandAtom);
    if (command === null) return;
    store.set(selectionCountAtom, renderer.applySelectionCommand(command));
    store.set(selectionCommandAtom, null);
  });

  // ---- Renderer to atoms. -------------------------------------------------------

  const pullCameraState = () => {
    store.set(
      cameraPositionAtom,
      new Vector3().copy(renderer.cameraController.position),
    );
    store.set(
      cameraRotationAtom,
      new Euler().copy(renderer.cameraController.rotation),
    );
  };
  renderer.cameraController.addEventListener("changed", pullCameraState);
  unsubscribers.push(() => {
    renderer.cameraController.removeEventListener("changed", pullCameraState);
  });

  return () => {
    for (const unsubscribe of unsubscribers) unsubscribe();
  };
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
