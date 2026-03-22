/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import js from "@eslint/js";
import react from "eslint-plugin-react";
import reactHooks from "eslint-plugin-react-hooks";
import reactRefresh from "eslint-plugin-react-refresh";
import { defineConfig } from "eslint/config";
import globals from "globals";
import tseslint from "typescript-eslint";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export default defineConfig({
  ignores: [".vite", "dist"],
  extends: [js.configs.recommended, ...tseslint.configs.recommendedTypeChecked],
  files: ["**/*.{ts,tsx}"],
  languageOptions: {
    ecmaVersion: 2023,
    globals: globals.browser,
    parserOptions: {
      project: ["./tsconfig.json"],
      tsconfigRootDir: import.meta.dirname,
    },
  },
  settings: { react: { version: "detect" } },
  plugins: {
    react,
    "react-hooks": reactHooks,
    "react-refresh": reactRefresh,
  },
  rules: {
    ...react.configs.recommended.rules,
    ...react.configs["jsx-runtime"].rules,
    ...reactHooks.configs.recommended.rules,
    "react-refresh/only-export-components": "off",
  },
});

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
