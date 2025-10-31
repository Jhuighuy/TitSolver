/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { create } from "zustand";

import { assert, type Side } from "~/utils";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

type MenuState = {
  size: number;
  setSize: (width: number) => void;
  activeItem: number;
  setActiveItem: (index: number) => void;
};

const createMenuStore = () =>
  create<MenuState>((set) => ({
    size: 320,
    setSize(size) {
      assert(size > 0, "Menu size must be positive!");
      set({ size });
    },
    activeItem: 0,
    setActiveItem(index) {
      set((state) => {
        const newIndex = index === state.activeItem ? -1 : index;
        return { activeItem: newIndex };
      });
    },
  }));

const menuStoreInstances: Record<Side, () => MenuState> = {
  left: createMenuStore(),
  right: createMenuStore(),
  top: createMenuStore(),
  bottom: createMenuStore(),
};

/** @todo Add tests once needed. Right not the logic it too simple. */
export function useMenuStore(side: Side) {
  return menuStoreInstances[side]();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
