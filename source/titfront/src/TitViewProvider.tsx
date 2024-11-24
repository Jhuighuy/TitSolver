/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import React, { createContext, useContext, useState, useEffect } from "react";
import { Module } from "./TitView";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

const TitViewContext = createContext<Module | null>(null);

/**
 * Use the the instance of the WASM module `tit_view_wasm`.
 */
export function useTitView(): Module {
  const module = useContext(TitViewContext);
  if (module === null) throw new Error("`TitViewProvider` is not initialized!");
  return module;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

enum LoadingState {
  loading,
  error,
  loaded,
}

/**
 * React context provider that provides the WASM module `tit_view_wasm` to its
 * children.
 */
export default function TitViewProvider({
  children,
}: {
  readonly children: React.ReactNode;
}) {
  const [loadingState, setLoadingState] = useState<LoadingState>(
    LoadingState.loading
  );
  const [error, setError] = useState<Error | null>(null);
  const [module, setModule] = useState<Module | null>(null);

  // Load the WASM module.
  useEffect(() => {
    let mounted = true;
    async function loadModule() {
      const module = (await import(
        new URL("tit_view_wasm.js", import.meta.url).href
      )) as typeof import("./TitView");
      if (!mounted) return;
      const moduleInstance = await module.default();
      setModule(moduleInstance);
      setLoadingState(LoadingState.loaded);
    }
    loadModule().catch((error: Error) => {
      setError(error);
      setLoadingState(LoadingState.error);
    });
    return () => {
      mounted = false;
    };
  }, []);

  /** @todo Add a proper loading indicator. */
  if (loadingState === LoadingState.loading) {
    return <output>Loading WASM module...</output>;
  }
  if (loadingState === LoadingState.error) {
    return <output>Failed to load WASM module: {error?.message}.</output>;
  }
  return (
    <TitViewContext.Provider value={module}>{children}</TitViewContext.Provider>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
