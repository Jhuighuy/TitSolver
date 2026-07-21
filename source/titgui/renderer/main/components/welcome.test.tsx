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
import { Welcome } from "~/renderer/main/components/welcome";
import { recentCasesAtom } from "~/renderer/main/state/case";
import type { CaseState } from "~/shared/case";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

const store = getDefaultStore();

// The result the next dialog-based flow resolves to (`null` = cancelled).
let nextCaseState: CaseState = null;
let openedRecents: string[] = [];
let fake: FakeIpc;

beforeAll(() => {
  fake = installFakeIpc({
    case: {
      newCase: () => nextCaseState,
      openCase: () => nextCaseState,
      openRecent: (_context, dir) => {
        openedRecents.push(dir);
        return nextCaseState;
      },
    },
  });
});

afterAll(() => {
  fake.uninstall();
});

beforeEach(() => {
  nextCaseState = null;
  openedRecents = [];
  store.set(recentCasesAtom, []);
});

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

describe("Welcome", () => {
  it("shows the product mark and an empty-recents hint", () => {
    renderWithProviders(<Welcome />);
    expect(screen.getByText("BlueTit Solver")).toBeVisible();
    expect(screen.getByText("Cases you open will show up here.")).toBeVisible();
  });

  it("lists the recent cases", () => {
    store.set(recentCasesAtom, [
      { dir: "/tmp/dam", name: "dam", lastOpenedAt: 0 },
      { dir: "/tmp/wave", name: "wave", lastOpenedAt: 1 },
    ]);
    renderWithProviders(<Welcome />);
    expect(screen.getByText("dam")).toBeVisible();
    expect(screen.getByText("/tmp/wave")).toBeVisible();
  });

  it("opens a recent case and notifies on success", async () => {
    const user = userEvent.setup();
    const onCaseOpened = vi.fn<() => void>();
    store.set(recentCasesAtom, [
      { dir: "/tmp/dam", name: "dam", lastOpenedAt: 0 },
    ]);
    nextCaseState = { dir: "/tmp/dam", name: "dam", dirty: false };
    renderWithProviders(<Welcome onCaseOpened={onCaseOpened} />);

    await user.click(screen.getByText("dam"));
    await vi.waitFor(() => {
      expect(openedRecents).toEqual(["/tmp/dam"]);
      expect(onCaseOpened).toHaveBeenCalledTimes(1);
    });
  });

  it("does not notify when the open dialog is cancelled", async () => {
    const user = userEvent.setup();
    const onCaseOpened = vi.fn<() => void>();
    renderWithProviders(<Welcome onCaseOpened={onCaseOpened} />);

    await user.click(screen.getByRole("button", { name: "Open Case…" }));
    // The flow resolves to null (cancelled) — no notification.
    await new Promise((resolve) => {
      setTimeout(resolve, 20);
    });
    expect(onCaseOpened).not.toHaveBeenCalled();
  });

  it("notifies after creating a new case", async () => {
    const user = userEvent.setup();
    const onCaseOpened = vi.fn<() => void>();
    nextCaseState = { dir: "/tmp/new", name: "new", dirty: false };
    renderWithProviders(<Welcome onCaseOpened={onCaseOpened} />);

    await user.click(screen.getByRole("button", { name: "New Case…" }));
    await vi.waitFor(() => {
      expect(onCaseOpened).toHaveBeenCalledTimes(1);
    });
  });
});

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
