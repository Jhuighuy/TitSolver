/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// @vitest-environment jsdom

import { screen } from "@testing-library/react";
import { userEvent } from "@testing-library/user-event";
import { afterAll, beforeAll, describe, expect, it, vi } from "vitest";

import { type FakeIpc, installFakeIpc } from "~/renderer/common/fake-ipc";
import { renderWithProviders } from "~/renderer/common/testing";
import { App } from "~/renderer/main/components/app";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// A whole-app smoke test: the real component tree over a fake IPC bridge
// that answers like a freshly started app with no case open.

const helpAddTab = vi.fn();
let fake: FakeIpc;

beforeAll(() => {
  fake = installFakeIpc({
    window: {
      persistGet: () => undefined,
      persistSet: () => {},
      isFullScreen: () => false,
    },
    theme: { get: () => "system", set: () => {} },
    case: {
      state: () => null,
      getSpec: () => ({ type: "record", fields: [] }),
      document: () => null,
      recents: () => [],
    },
    session: {
      isSolverRunning: () => false,
      frameCount: () => 0,
      frameTimes: () => [],
    },
    help: { addTab: helpAddTab },
  });
});

afterAll(() => {
  fake.uninstall();
});

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

describe("App", () => {
  it("renders the title bar, menus, and welcome screen", async () => {
    renderWithProviders(<App />);

    // Both menu bars are present.
    for (const name of [
      "Setup",
      "Storage",
      "Dashboard",
      "Help",
      "Settings",
      "Logs",
      "Output",
    ]) {
      expect(screen.getByRole("button", { name })).toBeVisible();
    }

    // With no case open, the workspace shows the welcome screen (which
    // shares the product name with the title bar).
    expect(
      await screen.findByText("Cases you open will show up here."),
    ).toBeVisible();
    expect(screen.getAllByText("BlueTit Solver").length).toBeGreaterThan(0);
  });

  it("opens, switches, and closes side menu panes", async () => {
    const user = userEvent.setup();
    renderWithProviders(<App />);

    // Open the Settings pane; its header and theme selector appear.
    const settingsButton = screen.getByRole("button", { name: "Settings" });
    await user.click(settingsButton);
    expect(settingsButton).toHaveAttribute("aria-pressed", "true");
    expect(screen.getByText("Appearance")).toBeVisible();

    // Switch to the Help pane.
    await user.click(screen.getByRole("button", { name: "Help" }));
    expect(settingsButton).toHaveAttribute("aria-pressed", "false");
    expect(screen.getByText("User Manual")).toBeVisible();

    // The pane close button deactivates the menu.
    await user.click(screen.getByRole("button", { name: "Close" }));
    expect(screen.getByRole("button", { name: "Help" })).toHaveAttribute(
      "aria-pressed",
      "false",
    );
  });

  it("toggles a bottom menu pane from its trigger", async () => {
    const user = userEvent.setup();
    renderWithProviders(<App />);

    const logsButton = screen.getByRole("button", { name: "Logs" });
    await user.click(logsButton);
    expect(logsButton).toHaveAttribute("aria-pressed", "true");

    await user.click(logsButton);
    expect(logsButton).toHaveAttribute("aria-pressed", "false");
  });

  it("opens the user manual from the help pane", async () => {
    const user = userEvent.setup();
    renderWithProviders(<App />);

    await user.click(screen.getByRole("button", { name: "Help" }));
    await user.click(screen.getByText("User Manual"));
    expect(helpAddTab).toHaveBeenCalled();
  });
});

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
