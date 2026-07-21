/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// @vitest-environment jsdom

import { screen } from "@testing-library/react";
import { userEvent } from "@testing-library/user-event";
import { getDefaultStore } from "jotai";
import {
  afterAll,
  beforeAll,
  beforeEach,
  describe,
  expect,
  it,
  vi,
} from "vitest";

import { type FakeIpc, installFakeIpc } from "~/renderer/common/fake-ipc";
import { renderWithProviders } from "~/renderer/common/testing";
import { Timeline } from "~/renderer/main/components/timeline";
import {
  frameIndexAtom,
  frameTimesAtom,
  numFramesAtom,
} from "~/renderer/main/state/storage";
import type { Frame } from "~/shared/storage";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

const store = getDefaultStore();

function makeFrame(): Frame {
  return {
    rho: {
      type: { kind: "float32_t", rank: 0, dim: 2 },
      data: new Float32Array([0]),
    },
    r: {
      type: { kind: "float32_t", rank: 1, dim: 2 },
      data: new Float32Array([0, 0]),
    },
  };
}

let frameRequests: number[] = [];
let fake: FakeIpc;

beforeAll(() => {
  fake = installFakeIpc({
    session: {
      frame: (_context, index) => {
        frameRequests.push(index);
        return makeFrame();
      },
    },
  });
});

afterAll(() => {
  fake.uninstall();
});

beforeEach(() => {
  frameRequests = [];
  store.set(numFramesAtom, 4);
  store.set(frameIndexAtom, 1);
  store.set(frameTimesAtom, [0, 0.5, 1, 1.5]);
});

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

describe("Timeline", () => {
  it("shows the physical time of the displayed frame", () => {
    renderWithProviders(<Timeline />);
    expect(screen.getByText("t = 0.5000")).toBeVisible();
  });

  it("hides the time readout when times are unknown", () => {
    store.set(frameTimesAtom, []);
    renderWithProviders(<Timeline />);
    expect(screen.queryByText(/t =/u)).not.toBeInTheDocument();
  });

  it("exposes slider semantics and scrubs with the keyboard", async () => {
    const user = userEvent.setup();
    renderWithProviders(<Timeline />);

    const slider = screen.getByRole("slider", { name: "Timeline" });
    expect(slider).toHaveAttribute("aria-valuemin", "0");
    expect(slider).toHaveAttribute("aria-valuemax", "3");
    expect(slider).toHaveAttribute("aria-valuenow", "1");

    slider.focus();
    await user.keyboard("{ArrowRight}");
    await vi.waitFor(() => {
      expect(frameRequests).toEqual([2]);
    });

    await user.keyboard("{Home}");
    await vi.waitFor(() => {
      expect(frameRequests).toEqual([2, 0]);
    });
  });

  it("steps between frames with the playback buttons", async () => {
    const user = userEvent.setup();
    renderWithProviders(<Timeline />);

    await user.click(screen.getByRole("button", { name: "Go to next step" }));
    await user.click(screen.getByRole("button", { name: "Go to end" }));
    await vi.waitFor(() => {
      expect(frameRequests).toEqual([2, 3]);
    });
  });

  it("disables navigation without frames", () => {
    store.set(numFramesAtom, 0);
    store.set(frameIndexAtom, null);
    store.set(frameTimesAtom, []);
    renderWithProviders(<Timeline />);

    expect(screen.getByRole("button", { name: "Play" })).toBeDisabled();
    expect(screen.getByRole("button", { name: "Go to end" })).toBeDisabled();
    expect(screen.getByRole("slider")).toHaveAttribute("aria-disabled", "true");
  });
});

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
