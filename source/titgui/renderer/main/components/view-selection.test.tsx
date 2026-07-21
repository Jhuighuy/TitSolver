/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// @vitest-environment jsdom

import { fireEvent, screen } from "@testing-library/react";
import { getDefaultStore } from "jotai";
import { Box2 } from "three";
import { beforeEach, describe, expect, it } from "vitest";

import { renderWithProviders } from "~/renderer/common/testing";
import { Polygon2 } from "~/renderer/common/visual/polygon2";
import type { SelectionCommand } from "~/renderer/common/visual/selection";
import { ViewSelection } from "~/renderer/main/components/view-selection";
import {
  createViewportState,
  type ViewportState,
  ViewportStateProvider,
} from "~/renderer/main/state/viewport";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

const store = getDefaultStore();

let viewport: ViewportState;
let commands: SelectionCommand[];

beforeEach(() => {
  viewport = createViewportState();
  commands = [];
  store.sub(viewport.selectionCommandAtom, () => {
    const command = store.get(viewport.selectionCommandAtom);
    if (command !== null) commands.push(command);
  });
});

function renderSelection() {
  const result = renderWithProviders(
    <ViewportStateProvider value={viewport}>
      <ViewSelection>
        <span>viewport content</span>
      </ViewSelection>
    </ViewportStateProvider>,
  );
  const wrapper = result.container.firstElementChild as HTMLElement;
  // The interactive overlay only exists while a selection tool is active.
  const overlay = () => {
    const element = result.container.querySelector(".size-full");
    expect(element).not.toBeNull();
    return element as HTMLElement;
  };
  return { ...result, wrapper, overlay };
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

describe("ViewSelection", () => {
  it("renders only the viewport content in normal mode", () => {
    renderSelection();
    expect(screen.getByText("viewport content")).toBeVisible();
    expect(screen.queryByText(/Select/u)).not.toBeInTheDocument();
  });

  it("drags a box selection and emits the marquee", () => {
    store.set(viewport.toolModeAtom, "rect");
    const { overlay, container } = renderSelection();
    expect(screen.getByText(/Box Select/u)).toBeVisible();

    fireEvent.pointerDown(overlay(), { clientX: 5, clientY: 5, pointerId: 1 });
    fireEvent.pointerMove(overlay(), {
      clientX: 40,
      clientY: 30,
      pointerId: 1,
    });
    // The marquee rectangle tracks the drag.
    const rect = container.querySelector("rect");
    expect(rect).toHaveAttribute("width", "35");
    expect(rect).toHaveAttribute("height", "25");

    fireEvent.pointerUp(overlay(), { clientX: 40, clientY: 30, pointerId: 1 });
    expect(commands).toHaveLength(1);
    const command = commands[0];
    expect(command.action).toBe("replace");
    if (command.action === "clear") return;
    expect(command.shape).toBeInstanceOf(Box2);
    const box = command.shape as Box2;
    expect(box.min.x).toBe(5);
    expect(box.max.y).toBe(30);
  });

  it("draws a lasso, skipping points closer than the threshold", () => {
    store.set(viewport.toolModeAtom, "lasso");
    const { overlay } = renderSelection();
    expect(screen.getByText(/Lasso Select/u)).toBeVisible();

    fireEvent.pointerDown(overlay(), { clientX: 0, clientY: 0, pointerId: 1 });
    fireEvent.pointerMove(overlay(), { clientX: 1, clientY: 1, pointerId: 1 });
    fireEvent.pointerMove(overlay(), {
      clientX: 10,
      clientY: 0,
      pointerId: 1,
    });
    fireEvent.pointerMove(overlay(), {
      clientX: 10,
      clientY: 10,
      pointerId: 1,
    });
    fireEvent.pointerUp(overlay(), { clientX: 10, clientY: 10, pointerId: 1 });

    expect(commands).toHaveLength(1);
    const command = commands[0];
    if (command.action === "clear") return;
    expect(command.shape).toBeInstanceOf(Polygon2);
    // The 1-pixel move is below the 4-pixel threshold and is dropped.
    const points = (command.shape as Polygon2).points.map(({ x, y }) => [x, y]);
    expect(points).toEqual([
      [0, 0],
      [10, 0],
      [10, 10],
    ]);
  });

  it("modifies the action with shift and alt", () => {
    store.set(viewport.toolModeAtom, "rect");
    const { wrapper, overlay } = renderSelection();

    fireEvent.keyDown(wrapper, { key: "Shift", shiftKey: true });
    expect(screen.getByText("Add")).toBeVisible();

    fireEvent.pointerDown(overlay(), { clientX: 0, clientY: 0, pointerId: 1 });
    fireEvent.pointerUp(overlay(), { clientX: 9, clientY: 9, pointerId: 1 });
    expect(commands.at(-1)?.action).toBe("add");

    fireEvent.keyUp(wrapper, { key: "Shift" });
    fireEvent.keyDown(wrapper, { key: "Alt", altKey: true });
    expect(screen.getByText("Subtract")).toBeVisible();
  });

  it("clears the selection and leaves the tool on escape", () => {
    store.set(viewport.toolModeAtom, "rect");
    store.set(viewport.selectionCountAtom, 2);
    const { wrapper } = renderSelection();
    expect(screen.getByText("2")).toBeVisible();
    expect(screen.getByText(/particles selected/u)).toBeVisible();

    fireEvent.keyDown(wrapper, { key: "Escape" });
    expect(store.get(viewport.toolModeAtom)).toBe("normal");
    expect(commands).toEqual([{ action: "clear" }]);
  });
});

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
