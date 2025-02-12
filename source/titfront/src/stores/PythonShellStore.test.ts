/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { act, renderHook } from "@testing-library/react";
import { beforeEach, describe, expect, it } from "vitest";

import { type Command, usePythonShellStore } from "~/stores/PythonShellStore";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

const commands: Command[] = [
  { input: "test1", status: "pending" },
  { input: "test2", status: "pending" },
  { input: "test3", status: "pending" },
];

beforeEach(() => usePythonShellStore.getState().reset());

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

describe("usePythonShellStore", () => {
  it("should push and update commands", () => {
    const { result } = renderHook(() => usePythonShellStore());
    for (const command of commands) {
      act(() => result.current.pushCommand(command));
    }
    expect(result.current.commands).toEqual(commands);

    const updatedCommand: Command = {
      input: "test1",
      status: "success",
      output: "output",
    };
    act(() => result.current.updateCommand(0, updatedCommand));
    expect(result.current.commands).toEqual([
      updatedCommand,
      ...commands.slice(1),
    ]);
  });

  it("should handle multiple pending commands", () => {
    const { result } = renderHook(() => usePythonShellStore());
    act(() => {
      for (const command of commands) result.current.pushCommand(command);
    });

    act(() =>
      result.current.updateCommand(1, {
        input: "test2",
        status: "success",
        output: "completed first",
      })
    );
    act(() =>
      result.current.updateCommand(0, {
        input: "test1",
        status: "success",
        output: "completed second",
      })
    );

    expect(result.current.commands[0].status).toBe("success");
    expect(result.current.commands[1].status).toBe("success");
    expect(result.current.commands[2].status).toBe("pending");
  });

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  it("should browse history up", () => {
    const { result } = renderHook(() => usePythonShellStore());
    for (const command of commands) {
      act(() => result.current.pushCommand(command));
    }

    // Upwards a few steps.
    for (let i = 2; i >= 0; i--) {
      act(() => result.current.historyUp());
      expect(result.current.historyIndex).toBe(i);
      expect(result.current.currentInput).toBe(commands[i].input);
    }

    // Browsing beyond the first command is noop.
    for (let i = 0; i < 10; i++) {
      act(() => result.current.historyUp());
      expect(result.current.historyIndex).toBe(0);
      expect(result.current.currentInput).toBe("test1");
    }
  });

  it("should not browse history up if user input is present", () => {
    const { result } = renderHook(() => usePythonShellStore());
    act(() => result.current.pushCommand(commands[0]));
    act(() => result.current.setCurrentInput("user input"));
    act(() => result.current.historyUp());
    expect(result.current.historyIndex).toBe(null);
    expect(result.current.currentInput).toBe("user input");
  });

  it("should not browse history up history is empty", () => {
    const { result } = renderHook(() => usePythonShellStore());
    act(() => result.current.historyUp());
    expect(result.current.historyIndex).toBe(null);
    expect(result.current.currentInput).toBe("");
  });

  it("should browse history down", () => {
    const { result } = renderHook(() => usePythonShellStore());
    act(() => {
      for (const command of commands) result.current.pushCommand(command);
    });

    // Browsing not started, so nothing happens.
    act(() => result.current.historyDown());
    expect(result.current.historyIndex).toBe(null);
    expect(result.current.currentInput).toBe("");

    // Browsing history upwards and then downwards.
    for (let i = 0; i < 10; i++) act(() => result.current.historyUp());
    for (let i = 1; i <= 2; i++) {
      act(() => result.current.historyDown());
      expect(result.current.historyIndex).toBe(i);
      expect(result.current.currentInput).toBe(commands[i].input);
    }

    // Browsing beyond the last command cancels history browsing.
    act(() => result.current.historyDown());
    expect(result.current.historyIndex).toBe(null);
    expect(result.current.currentInput).toBe("");
  });

  it("should cancel browsing if user changes input", () => {
    const { result } = renderHook(() => usePythonShellStore());
    act(() => {
      for (const command of commands) result.current.pushCommand(command);
    });

    // Browsing history upwards and then changing input.
    act(() => result.current.historyUp());
    expect(result.current.historyIndex).toBe(2);
    expect(result.current.currentInput).toBe("test3");
    act(() => result.current.setCurrentInput("new input"));
    expect(result.current.historyIndex).toBe(null);
    expect(result.current.currentInput).toBe("new input");
  });

  it("should handle rapid keyboard navigation", () => {
    const { result } = renderHook(() => usePythonShellStore());
    act(() => {
      for (const command of commands) result.current.pushCommand(command);
    });

    act(() => {
      result.current.historyUp();
      result.current.historyUp();
      result.current.historyDown();
      result.current.historyDown();
    });

    expect(result.current.currentInput).toBe("");
    expect(result.current.historyIndex).toBe(null);
  });

  it("should handle keyboard interrupts during history browsing", () => {
    const { result } = renderHook(() => usePythonShellStore());
    act(() => {
      for (const command of commands) result.current.pushCommand(command);
    });

    act(() => result.current.historyUp());
    act(() =>
      result.current.updateCommand(1, {
        input: "test2",
        status: "failure",
      })
    );

    expect(result.current.currentInput).toBe("test3");
  });

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  it("should clear output", () => {
    const { result } = renderHook(() => usePythonShellStore());
    act(() => result.current.pushCommand(commands[0]));
    act(() => result.current.clearVisibleCommands());
    act(() => result.current.pushCommand(commands[1]));
    expect(result.current.firstVisibleCommand).toBe(1);
  });

  it("should not clear current input when clearing output", () => {
    const { result } = renderHook(() => usePythonShellStore());
    act(() => result.current.setCurrentInput("user input"));
    act(() => result.current.clearVisibleCommands());
    expect(result.current.currentInput).toBe("user input");
  });

  it("should not cancel history browsing when clearing output", () => {
    const { result } = renderHook(() => usePythonShellStore());
    for (const command of commands) {
      act(() => result.current.pushCommand(command));
    }

    act(() => result.current.historyUp());
    act(() => result.current.clearVisibleCommands());
    expect(result.current.historyIndex).toBe(2);
    expect(result.current.currentInput).toBe("test3");
  });
});

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
