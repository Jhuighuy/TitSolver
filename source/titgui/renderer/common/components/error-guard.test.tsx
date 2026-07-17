/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// @vitest-environment jsdom

import { render, screen } from "@testing-library/react";
import { userEvent } from "@testing-library/user-event";
import { describe, expect, it, vi } from "vitest";

import { ErrorGuard } from "~/renderer/common/components/error-guard";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Throws until the flag is flipped. The failure must be persistent: React
// retries a render that crashed concurrently, so a throw-once component
// would self-heal without ever reaching the boundary.
let shouldThrow = false;

// Swallow the error event React dispatches while handling the throw.
function onWindowError(event: ErrorEvent) {
  event.preventDefault();
}

function FlakyRegion() {
  if (shouldThrow) throw new Error("Region exploded.");
  return <div>Region content.</div>;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

describe("ErrorGuard", () => {
  it("renders children when nothing fails", () => {
    render(
      <ErrorGuard>
        <div>All good.</div>
      </ErrorGuard>,
    );
    expect(screen.getByText("All good.")).toBeVisible();
  });

  it("catches render errors and recovers on retry", async () => {
    const user = userEvent.setup();
    shouldThrow = true;

    // React reports caught render errors to the console and the window;
    // keep the test output quiet.
    const consoleError = vi
      .spyOn(console, "error")
      .mockImplementation(() => {});
    window.addEventListener("error", onWindowError);
    try {
      render(
        <ErrorGuard>
          <FlakyRegion />
        </ErrorGuard>,
      );

      expect(
        screen.getByText("This part of the window ran into an error."),
      ).toBeVisible();
      expect(screen.getByText("Region exploded.")).toBeVisible();

      shouldThrow = false;
      await user.click(screen.getByRole("button", { name: "Try Again" }));
      expect(screen.getByText("Region content.")).toBeVisible();
    } finally {
      window.removeEventListener("error", onWindowError);
      consoleError.mockRestore();
    }
  });
});

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
