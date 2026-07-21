/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// @vitest-environment jsdom

import { screen, within } from "@testing-library/react";
import { userEvent } from "@testing-library/user-event";
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
import { Workspace } from "~/renderer/main/components/workspace";

// The workspace shell is under test, not the GL viewport or the timeline.
vi.mock("~/renderer/main/components/viewport", () => ({
  Viewport: () => <div data-testid="viewport" />,
}));
vi.mock("~/renderer/main/components/timeline", () => ({
  Timeline: () => <div data-testid="timeline" />,
}));

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// In-memory per-window persisted state.
let persisted = new Map<string, unknown>();
let fake: FakeIpc;

beforeAll(() => {
  fake = installFakeIpc({
    window: {
      persistGet: (_context, key) => persisted.get(key),
      persistSet: (_context, key, value) => {
        persisted.set(key, value);
      },
      isFullScreen: () => false,
    },
    case: {
      recents: () => [],
    },
  });
});

afterAll(() => {
  fake.uninstall();
});

beforeEach(() => {
  persisted = new Map();
});

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

describe("Workspace", () => {
  it("opens with the welcome and viewport tabs, welcome active", () => {
    renderWithProviders(<Workspace />);
    const tabs = screen.getAllByRole("tab");
    expect(tabs.map((tab) => tab.textContent)).toEqual(["Welcome", "Viewport"]);
    expect(screen.getByText("BlueTit Solver")).toBeVisible();
  });

  it("switches tabs and persists the workspace layout", async () => {
    const user = userEvent.setup();
    renderWithProviders(<Workspace />);

    await user.click(screen.getByRole("tab", { name: "Viewport" }));
    expect(screen.getByTestId("viewport")).toBeVisible();
    expect(screen.getByTestId("timeline")).toBeVisible();

    await vi.waitFor(() => {
      expect(persisted.get("workspace")).toMatchObject({
        active: "viewport-1",
      });
    });
  });

  it("closes tabs down to the empty-workspace screen and reopens", async () => {
    const user = userEvent.setup();
    renderWithProviders(<Workspace />);

    for (const tab of screen.getAllByRole("tab")) {
      // eslint semantics: close buttons live inside the tabs.
      // oxlint-disable-next-line no-await-in-loop -- sequential UI actions.
      await user.click(within(tab).getByRole("button", { name: "Close Tab" }));
    }
    expect(screen.getByText("All tabs are closed")).toBeVisible();

    await user.click(screen.getByRole("button", { name: "Welcome" }));
    expect(screen.getByRole("tab", { name: "Welcome" })).toBeVisible();
  });

  it("adds a numbered viewport from the add-tab menu", async () => {
    const user = userEvent.setup();
    renderWithProviders(<Workspace />);

    await user.click(screen.getByRole("button", { name: "Add Tab" }));
    await user.click(
      await screen.findByRole("menuitem", { name: "New Viewport" }),
    );

    const tabs = screen.getAllByRole("tab");
    expect(tabs.map((tab) => tab.textContent)).toEqual([
      "Welcome",
      "Viewport 1",
      "Viewport 2",
    ]);
  });

  it("restores the persisted layout", async () => {
    persisted.set("workspace", {
      tabs: [{ id: "viewport-1", kind: "viewport" }],
      active: "viewport-1",
    });
    renderWithProviders(<Workspace />);

    await vi.waitFor(() => {
      const tabs = screen.getAllByRole("tab");
      expect(tabs.map((tab) => tab.textContent)).toEqual(["Viewport"]);
      expect(screen.getByTestId("viewport")).toBeVisible();
    });
  });
});

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
