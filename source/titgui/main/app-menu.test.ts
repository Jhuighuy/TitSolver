/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { Menu, type MenuItemConstructorOptions } from "electron";
import { beforeEach, describe, expect, it, vi } from "vitest";

import { type AppMenuHandlers, updateApplicationMenu } from "~/main/app-menu";

vi.mock("electron");

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

function makeHandlers(): AppMenuHandlers {
  return {
    newCase: vi.fn<() => void>(),
    openCase: vi.fn<() => void>(),
    openRecentCase: vi.fn<(dir: string) => void>(),
    saveCase: vi.fn<() => void>(),
    closeCase: vi.fn<() => void>(),
    openHelp: vi.fn<() => void>(),
  };
}

// The mock `Menu.buildFromTemplate` returns `{ items: template }`.
function installedTemplate(): MenuItemConstructorOptions[] {
  const menu = vi.mocked(Menu.setApplicationMenu).mock.calls.at(-1)?.[0];
  return (menu as unknown as { items: MenuItemConstructorOptions[] }).items;
}

function fileMenu() {
  const file = installedTemplate().find(({ label }) => label === "File");
  return file?.submenu as MenuItemConstructorOptions[];
}

beforeEach(() => {
  vi.clearAllMocks();
});

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

describe("updateApplicationMenu", () => {
  it("builds the standard menus", () => {
    updateApplicationMenu(makeHandlers(), { caseState: null, recents: [] });
    const labels = installedTemplate().map((item) => item.label ?? item.role);
    expect(labels).toContain("File");
    expect(labels).toContain("editMenu");
    expect(labels).toContain("Help");
  });

  it("disables case items without an open case", () => {
    updateApplicationMenu(makeHandlers(), { caseState: null, recents: [] });
    const save = fileMenu().find(({ label }) => label === "Save Case");
    const close = fileMenu().find(({ label }) => label === "Close Case");
    expect(save?.enabled).toBe(false);
    expect(close?.enabled).toBe(false);
  });

  it("enables case items and wires the handlers", () => {
    const handlers = makeHandlers();
    updateApplicationMenu(handlers, {
      caseState: { dir: "/tmp/dam", name: "dam", dirty: true },
      recents: [],
    });

    const save = fileMenu().find(({ label }) => label === "Save Case");
    expect(save?.enabled).toBe(true);
    (save?.click as (() => void) | undefined)?.();
    expect(handlers.saveCase).toHaveBeenCalledTimes(1);

    const create = fileMenu().find(({ label }) => label === "New Case…");
    (create?.click as (() => void) | undefined)?.();
    expect(handlers.newCase).toHaveBeenCalledTimes(1);
  });

  it("lists the recents and opens them on click", () => {
    const handlers = makeHandlers();
    updateApplicationMenu(handlers, {
      caseState: null,
      recents: [
        { dir: "/tmp/dam", name: "dam", lastOpenedAt: 0 },
        { dir: "/tmp/wave", name: "wave", lastOpenedAt: 1 },
      ],
    });

    const recent = fileMenu().find(({ label }) => label === "Open Recent");
    expect(recent?.enabled).toBe(true);
    const entries = recent?.submenu as MenuItemConstructorOptions[];
    expect(entries.map(({ label }) => label)).toEqual(["dam", "wave"]);

    (entries[1].click as () => void)();
    expect(handlers.openRecentCase).toHaveBeenCalledWith("/tmp/wave");
  });

  it("disables the recents submenu when empty", () => {
    updateApplicationMenu(makeHandlers(), { caseState: null, recents: [] });
    const recent = fileMenu().find(({ label }) => label === "Open Recent");
    expect(recent?.enabled).toBe(false);
  });
});

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
