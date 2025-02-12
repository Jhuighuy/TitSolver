/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { create } from "zustand";

import { assert } from "~/utils";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export type Command = {
  input: string;
  status: "pending" | "success" | "failure";
  output?: string;
};

/** @todo Limit a command's history to a certain number of commands. */
type ShellState = {
  commands: Command[];
  firstVisibleCommand: number;
  currentInput: string;
  historyIndex: number | null;
  reset: () => void;
  pushCommand: (command: Command) => void;
  updateCommand: (index: number, command: Command) => void;
  clearVisibleCommands: () => void;
  setCurrentInput: (input: string) => void;
  historyUp: () => void;
  historyDown: () => void;
};

export const usePythonShellStore = create<ShellState>((set) => ({
  commands: [],
  firstVisibleCommand: 0,
  currentInput: "",
  historyIndex: null,
  reset() {
    set({
      commands: [],
      firstVisibleCommand: 0,
      currentInput: "",
      historyIndex: null,
    });
  },
  pushCommand(command: Command) {
    set((state) => ({ commands: [...state.commands, command] }));
  },
  updateCommand(index: number, command: Command) {
    set((state) => {
      const commands = [...state.commands];
      assert(index < commands.length, "Invalid command index!");
      assert(command.input === commands[index].input, "Input mismatch!");
      commands[index] = command;
      return { commands };
    });
  },
  clearVisibleCommands() {
    set((state) => ({ firstVisibleCommand: state.commands.length }));
  },
  setCurrentInput(input: string) {
    // Any input cancels history browsing.
    set({ currentInput: input, historyIndex: null });
  },
  historyUp() {
    set((state) => {
      let newIndex;
      if (state.historyIndex === null) {
        // If no user input is present and history is not empty, we can start
        // browsing it from the last command upwards.
        if (state.commands.length == 0 || state.currentInput) return state;
        newIndex = state.commands.length - 1;
      } else {
        // Browsing history upwards.
        newIndex = Math.max(0, state.historyIndex - 1);
      }
      return {
        currentInput: state.commands[newIndex].input,
        historyIndex: newIndex,
      };
    });
  },
  historyDown() {
    set((state) => {
      if (state.historyIndex === null) {
        // Can browse down only if we're browsing already.
        return state;
      }
      if (state.historyIndex < state.commands.length - 1) {
        // Browsing history downwards.
        const newIndex = state.historyIndex + 1;
        return {
          currentInput: state.commands[newIndex].input,
          historyIndex: newIndex,
        };
      }
      // Browsing beyond the last command cancels history browsing.
      return { currentInput: "", historyIndex: null };
    });
  },
}));

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
