/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import "@testing-library/jest-dom/vitest";
import { render, screen, waitFor } from "@testing-library/react";
import userEvent from "@testing-library/user-event";
import { beforeEach, describe, expect, it } from "vitest";

import { PyConnectionProvider } from "~/components/Python";
import { PythonShell } from "~/components/PythonShell";
import { usePythonShellStore } from "~/stores/PythonShellStore";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

beforeEach(() => usePythonShellStore.getState().reset());

async function setupPythonShell() {
  const result = render(
    <PyConnectionProvider>
      <PythonShell />
    </PyConnectionProvider>
  );
  await waitFor(() => {
    expect(screen.getByTestId("python-shell")).toBeInTheDocument();
  });
  return result;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Note: unfortunately, we cannot test user typing here, `user.keyboard` does
// not work with `contentEditable` elements.
describe("PythonShell", () => {
  it("renders commands", async () => {
    usePythonShellStore.getState().commands = [
      { input: "test1", status: "success", output: "output" },
      { input: "test2", status: "failure", output: "error" },
      { input: "", status: "success" }, // should render no output.
      { input: "test3", status: "pending" },
    ];
    const { asFragment } = await setupPythonShell();
    expect(asFragment()).toMatchSnapshot();
  });

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  it("clears visible commands", async () => {
    usePythonShellStore.getState().commands = [
      { input: "test", status: "success", output: "output" },
    ];
    await setupPythonShell();

    const user = userEvent.setup();
    await user.click(screen.getByTestId("python-shell"));
    await user.keyboard("{Control>}{k}{/Control}");
    expect(screen.queryByText("success")).not.toBeInTheDocument();
  });

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  it("browses history up", async () => {
    usePythonShellStore.getState().commands = [
      { input: "test1", status: "success", output: "output" },
      { input: "test2", status: "success", output: "output" },
      { input: "test3", status: "success", output: "output" },
    ];
    usePythonShellStore.getState().clearVisibleCommands();
    await setupPythonShell();

    const user = userEvent.setup();
    await user.click(screen.getByTestId("python-shell"));
    await user.keyboard("{ArrowUp}{ArrowUp}");
    expect(screen.getByText("test2")).toBeInTheDocument();
  });

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  it("browses history down", async () => {
    usePythonShellStore.getState().commands = [
      { input: "test1", status: "success", output: "output" },
      { input: "test2", status: "success", output: "output" },
      { input: "test3", status: "success", output: "output" },
    ];
    usePythonShellStore.getState().clearVisibleCommands();
    await setupPythonShell();

    const user = userEvent.setup();
    await user.click(screen.getByTestId("python-shell"));
    await user.keyboard("{ArrowUp}{ArrowUp}{ArrowDown}");
    expect(screen.getByText("test3")).toBeInTheDocument();
  });

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  it("runs commands and displays output", async () => {
    usePythonShellStore.getState().currentInput = "2 + 3";
    usePythonShellStore.getState().clearVisibleCommands();
    await setupPythonShell();

    const user = userEvent.setup();
    await user.click(screen.getByTestId("python-shell"));
    await user.keyboard("{Enter}");
    expect(screen.getByText("2 + 3")).toBeInTheDocument();
    await waitFor(() => {
      expect(screen.getByText("5")).toBeInTheDocument();
    });
  });

  it("does not run empty commands", async () => {
    usePythonShellStore.getState().currentInput = "";
    usePythonShellStore.getState().clearVisibleCommands();
    const { asFragment } = await setupPythonShell();

    const user = userEvent.setup();
    await user.click(screen.getByTestId("python-shell"));
    await user.keyboard("{Enter}");
    expect(asFragment()).toMatchSnapshot();
  });

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  it("runs commands and displays errors", async () => {
    usePythonShellStore.getState().currentInput = "1 / 0";
    usePythonShellStore.getState().clearVisibleCommands();
    await setupPythonShell();

    const user = userEvent.setup();
    await user.click(screen.getByTestId("python-shell"));
    await user.keyboard("{Enter}");
    expect(screen.getByText("1 / 0")).toBeInTheDocument();
    await waitFor(() => {
      expect(
        screen.getByText("ZeroDivisionError: division by zero")
      ).toBeInTheDocument();
    });
  });
});

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
