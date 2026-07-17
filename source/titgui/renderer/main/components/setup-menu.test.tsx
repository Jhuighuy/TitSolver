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
import { SetupMenu } from "~/renderer/main/components/setup-menu";
import {
  caseDocumentAtom,
  caseSpecAtom,
  caseStateAtom,
} from "~/renderer/main/state/case";
import type { CaseDocument, SpecJson, TreeJson } from "~/shared/case";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

const store = getDefaultStore();

// A small specification exercising every implemented editor kind.
const spec: SpecJson = {
  type: "record",
  fields: [
    { id: "schema", name: "Schema Version", spec: { type: "int", default: 1 } },
    {
      id: "simulation",
      name: "Simulation",
      spec: {
        type: "record",
        fields: [
          {
            id: "title",
            name: "Title",
            spec: { type: "string", default: "Untitled" },
          },
          {
            id: "gravity",
            name: "Gravity",
            spec: { type: "real", min: 0, default: 9.81, unit: "m/s^2" },
          },
          {
            id: "verbose",
            name: "Verbose",
            spec: { type: "bool", default: false },
          },
        ],
      },
    },
  ],
};

function makeDocument(
  authored: TreeJson,
  materialized: TreeJson,
  issues: CaseDocument["materialized"]["issues"] = [],
): CaseDocument {
  return {
    revision: 7,
    authored,
    materialized: { tree: materialized, issues, namespaces: {} },
  };
}

const defaultMaterialized: TreeJson = {
  schema: 1,
  simulation: { title: "Untitled", gravity: 9.81, verbose: false },
};

let updateCalls: { tree: TreeJson; revision: number }[] = [];
let fake: FakeIpc;

beforeAll(() => {
  fake = installFakeIpc({
    case: {
      updateTree: (_context, tree, revision) => {
        updateCalls.push({ tree, revision });
        return true;
      },
    },
  });
});

afterAll(() => {
  fake.uninstall();
});

beforeEach(() => {
  updateCalls = [];
  store.set(caseStateAtom, { dir: "/tmp/dam", name: "dam", dirty: false });
  store.set(caseSpecAtom, spec);
  store.set(caseDocumentAtom, makeDocument({ schema: 1 }, defaultMaterialized));
});

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

describe("SetupMenu", () => {
  it("shows a placeholder without an open case", () => {
    store.set(caseStateAtom, null);
    store.set(caseDocumentAtom, null);
    renderWithProviders(<SetupMenu />);
    expect(
      screen.getByText("Open a case to edit its setup."),
    ).toBeInTheDocument();
  });

  it("renders materialized values with units, hiding the schema field", () => {
    renderWithProviders(<SetupMenu />);
    expect(screen.getByRole("textbox", { name: "Title" })).toHaveValue(
      "Untitled",
    );
    expect(screen.getByRole("textbox", { name: "Gravity" })).toHaveValue(
      "9.81",
    );
    expect(screen.getByText("m/s^2")).toBeInTheDocument();
    expect(screen.getByRole("switch")).not.toBeChecked();
    expect(screen.queryByText("Schema Version")).not.toBeInTheDocument();
  });

  it("marks explicitly-set fields and resets them", async () => {
    const user = userEvent.setup();
    store.set(
      caseDocumentAtom,
      makeDocument(
        { schema: 1, simulation: { gravity: 5 } },
        {
          schema: 1,
          simulation: { title: "Untitled", gravity: 5, verbose: false },
        },
      ),
    );
    renderWithProviders(<SetupMenu />);

    // Only the authored field carries the reset control.
    expect(
      screen.queryByRole("button", { name: "Reset Title to default" }),
    ).not.toBeInTheDocument();
    const reset = screen.getByRole("button", {
      name: "Reset Gravity to default",
    });

    // Resetting deletes the authored node and submits the revision.
    await user.click(reset);
    await vi.waitFor(() => {
      expect(updateCalls).toEqual([{ tree: { schema: 1 }, revision: 7 }]);
    });
  });

  it("commits text edits on blur", async () => {
    const user = userEvent.setup();
    renderWithProviders(<SetupMenu />);

    const title = screen.getByRole("textbox", { name: "Title" });
    await user.clear(title);
    await user.type(title, "Dam Break");
    await user.tab();

    await vi.waitFor(() => {
      expect(updateCalls).toEqual([
        {
          tree: { schema: 1, simulation: { title: "Dam Break" } },
          revision: 7,
        },
      ]);
    });
  });

  it("toggles boolean fields immediately", async () => {
    const user = userEvent.setup();
    renderWithProviders(<SetupMenu />);

    await user.click(screen.getByRole("switch"));
    await vi.waitFor(() => {
      expect(updateCalls).toEqual([
        {
          tree: { schema: 1, simulation: { verbose: true } },
          revision: 7,
        },
      ]);
    });
  });

  it("shows field issues in place and unmatched issues under Problems", () => {
    store.set(
      caseDocumentAtom,
      makeDocument({ schema: 1 }, defaultMaterialized, [
        {
          code: "below_minimum",
          path: "/simulation/gravity",
          message: "Value -1 is below minimum 0.",
        },
        {
          code: "unknown_field",
          path: "/",
          message: "Unknown field 'bogus'.",
        },
      ]),
    );
    renderWithProviders(<SetupMenu />);

    expect(screen.getByText("Value -1 is below minimum 0.")).toBeVisible();
    expect(screen.getByText("Problems")).toBeVisible();
    expect(screen.getByText(/Unknown field 'bogus'/u)).toBeVisible();
  });

  it("registers the Save Case action, disabled while clean", () => {
    const { menuActions } = renderWithProviders(<SetupMenu />);
    const save = menuActions.find(({ name }) => name === "Save Case");
    expect(save?.disabled).toBe(true);
  });

  it("enables the Save Case action for a dirty case", () => {
    store.set(caseStateAtom, { dir: "/tmp/dam", name: "dam", dirty: true });
    const { menuActions } = renderWithProviders(<SetupMenu />);
    const save = menuActions.find(({ name }) => name === "Save Case");
    expect(save?.disabled).toBe(false);
  });
});

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
