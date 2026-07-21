/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { type Atom, atom, type Getter, type WritableAtom } from "jotai";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
 * A writable atom whose value is remembered independently per scope.
 *
 * The current scope is identified by the value of `scopeKeyAtom`. Reading an
 * atom in a scope it has never been written in yields `fallback`; writing
 * stores the value for the current scope only. Used for view settings that
 * should be remembered per displayed field (e.g. the chosen color map).
 */
export function scopedAtom<Value>(
  scopeKeyAtom: Atom<string>,
  fallback: (get: Getter) => Value,
): WritableAtom<Value, [Value], void> {
  const valuesAtom = atom<ReadonlyMap<string, Value>>(new Map());
  return atom(
    (get) => {
      const values = get(valuesAtom);
      const key = get(scopeKeyAtom);
      return values.has(key) ? (values.get(key) as Value) : fallback(get);
    },
    (get, set, value: Value) => {
      set(
        valuesAtom,
        new Map([...get(valuesAtom), [get(scopeKeyAtom), value]]),
      );
    },
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
