/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// @vitest-environment jsdom

import { screen } from "@testing-library/react";
import { userEvent } from "@testing-library/user-event";
import { getDefaultStore } from "jotai";
import { beforeEach, describe, expect, it } from "vitest";

import { renderWithProviders } from "~/renderer/common/testing";
import { FieldMap } from "~/renderer/common/visual/fields";
import { ViewControls } from "~/renderer/main/components/view-controls";
import {
  createViewportState,
  frameDataAtom,
  type ViewportState,
  ViewportStateProvider,
} from "~/renderer/main/state/viewport";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

const store = getDefaultStore();

let viewport: ViewportState;

beforeEach(() => {
  viewport = createViewportState();
  // Two particles in 2D: scalar rho, vector r/v.
  store.set(
    frameDataAtom,
    new FieldMap({
      rho: new Float32Array(2),
      r: new Float32Array(4),
      v: new Float32Array(4),
    }),
  );
});

function renderControls() {
  return renderWithProviders(
    <ViewportStateProvider value={viewport}>
      <ViewControls />
    </ViewportStateProvider>,
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

describe("ViewControls", () => {
  it("shows the control groups", () => {
    renderControls();
    for (const group of ["Interact", "View", "Display", "Render"]) {
      expect(screen.getByText(group)).toBeVisible();
    }
  });

  it("switches the interaction tool", async () => {
    const user = userEvent.setup();
    renderControls();

    await user.click(screen.getByText("Interact"));
    await user.click(await screen.findByText("Box Select"));
    expect(store.get(viewport.toolModeAtom)).toBe("rect");
  });

  it("selects the displayed field", async () => {
    const user = userEvent.setup();
    renderControls();

    await user.click(screen.getByText("Display"));
    // The field select shows the current field of this viewport.
    const fieldSelect = await screen.findByRole("combobox", {
      name: /Field/u,
    });
    await user.click(fieldSelect);
    await user.click(await screen.findByRole("option", { name: /v/u }));

    expect(store.get(viewport.fieldNameAtom)).toBe("v");
    // Selecting a vector field switches the representation to glyphs.
    expect(store.get(viewport.renderModeAtom)).toBe("glyphs");
  });

  it("toggles the legend per viewport", async () => {
    const user = userEvent.setup();
    renderControls();

    await user.click(screen.getByText("Render"));
    await user.click(await screen.findByRole("switch"));
    expect(store.get(viewport.legendEnabledAtom)).toBe(false);

    // A second viewport bundle is unaffected.
    expect(store.get(createViewportState().legendEnabledAtom)).toBe(true);
  });
});

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
