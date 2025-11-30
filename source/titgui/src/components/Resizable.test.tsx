/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import "@testing-library/jest-dom/vitest";
import { fireEvent, render, screen } from "@testing-library/react";
import { describe, expect, it, vi } from "vitest";

import { Resizable } from "~/components/Resizable";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

describe("Resizable", () => {
  it("renders children correctly", () => {
    const { getByText } = render(
      <Resizable side="left" size={200} setSize={() => {}}>
        <div>Test</div>
      </Resizable>
    );
    expect(getByText("Test")).toBeInTheDocument();
  });

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  it("applies initial width correctly", () => {
    render(
      <Resizable side="left" size={200} setSize={() => {}}>
        <div>Test</div>
      </Resizable>
    );
    expect(screen.getByTestId("resizable-content")).toHaveStyle("width: 200px");
  });

  it("applies initial height correctly", () => {
    render(
      <Resizable side="bottom" size={200} setSize={() => {}}>
        <div>Test</div>
      </Resizable>
    );
    expect(screen.getByTestId("resizable-content")).toHaveStyle(
      "height: 200px"
    );
  });

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  it("handles width changes with mouse correctly", () => {
    const setSize = vi.fn();
    render(
      <Resizable side="left" size={200} setSize={setSize}>
        <div>Test Content</div>
      </Resizable>
    );

    const resizer = screen.getByTestId("resizable-resizer");

    fireEvent.mouseDown(resizer, { clientX: 0 });
    fireEvent.mouseMove(window, { clientX: 50 });
    expect(document.body.style.cursor).toBe("ew-resize");
    expect(setSize).toHaveBeenCalledWith(250);

    fireEvent.mouseUp(window);
    expect(document.body.style.cursor).toBe("");
  });

  it("handles height changes with mouse correctly", () => {
    const setSize = vi.fn();
    render(
      <Resizable side="bottom" size={200} setSize={setSize}>
        <div>Test Content</div>
      </Resizable>
    );
    const resizer = screen.getByTestId("resizable-resizer");

    fireEvent.mouseDown(resizer, { clientY: 0 });
    fireEvent.mouseMove(window, { clientY: -50 });
    expect(document.body.style.cursor).toBe("ns-resize");
    expect(setSize).toHaveBeenCalledWith(250);

    fireEvent.mouseUp(window);
    expect(document.body.style.cursor).toBe("");
  });

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  it("respects size constraints", () => {
    const setSize = vi.fn();
    render(
      <Resizable
        side="left"
        size={200}
        setSize={setSize}
        minSize={100}
        maxSize={300}
      >
        <div>Test Content</div>
      </Resizable>
    );
    const resizer = screen.getByTestId("resizable-resizer");

    fireEvent.mouseDown(resizer, { clientX: 0 });
    fireEvent.mouseMove(window, { clientX: 50 });
    expect(setSize).toHaveBeenCalledWith(250);

    fireEvent.mouseMove(window, { clientX: 150 });
    expect(setSize).toHaveBeenCalledWith(300);

    fireEvent.mouseUp(window);

    fireEvent.mouseDown(resizer, { clientX: 0 });
    fireEvent.mouseMove(window, { clientX: -50 });
    expect(setSize).toHaveBeenCalledWith(150);

    fireEvent.mouseMove(window, { clientX: -150 });
    expect(setSize).toHaveBeenCalledWith(100);

    fireEvent.mouseUp(window);
  });
});

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
