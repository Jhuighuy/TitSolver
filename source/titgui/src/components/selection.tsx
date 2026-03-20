/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import {
  createContext,
  type ReactNode,
  useContext,
  useMemo,
  useState,
} from "react";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export type SelectionModifier = "replace" | "add" | "subtract";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export function mergeParticleSelection(
  previous: readonly number[],
  next: readonly number[],
  modifier: SelectionModifier,
): number[] {
  const previousSet = new Set(previous);
  const nextSet = new Set(next);

  switch (modifier) {
    case "replace":
      return [...nextSet].sort((a, b) => a - b);
    case "add":
      return [...new Set([...previousSet, ...nextSet])].sort((a, b) => a - b);
    case "subtract":
      return previous.filter((index) => !nextSet.has(index));
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

type SelectionContextValue = {
  selectedParticleIndices: number[];
  selectionCount: number;
  setSelection: (indices: number[]) => void;
  updateSelection: (indices: number[], modifier: SelectionModifier) => void;
  clearSelection: () => void;
};

const SelectionContext = createContext<SelectionContextValue | null>(null);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export function SelectionProvider({
  children,
}: Readonly<{ children: ReactNode }>) {
  const [selectedParticleIndices, setSelectedParticleIndices] = useState<
    number[]
  >([]);

  const value = useMemo<SelectionContextValue>(
    () => ({
      selectedParticleIndices,
      selectionCount: selectedParticleIndices.length,
      setSelection: (indices) =>
        setSelectedParticleIndices(
          mergeParticleSelection([], indices, "replace"),
        ),
      updateSelection: (indices, modifier) =>
        setSelectedParticleIndices((previous) =>
          mergeParticleSelection(previous, indices, modifier),
        ),
      clearSelection: () => setSelectedParticleIndices([]),
    }),
    [selectedParticleIndices],
  );

  return (
    <SelectionContext.Provider value={value}>
      {children}
    </SelectionContext.Provider>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export function useSelection() {
  const context = useContext(SelectionContext);
  if (context === null) {
    throw new Error("Selection context is not available");
  }
  return context;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
