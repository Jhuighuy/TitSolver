/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import js from "@eslint/js";
import react from "eslint-plugin-react";
import reactHooks from "eslint-plugin-react-hooks";
import reactRefresh from "eslint-plugin-react-refresh";
import unicorn from "eslint-plugin-unicorn";
import { defineConfig } from "eslint/config";
import globals from "globals";
import tseslint from "typescript-eslint";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export default defineConfig(
  {
    ignores: [
      "**/*.d.ts",
      ".vite/**",
      "coverage/**",
      "dist/**",
      "eslint.config.mjs",
      "prettier.config.mjs",
    ],
  },

  js.configs.recommended,
  tseslint.configs.strictTypeChecked,
  tseslint.configs.stylisticTypeChecked,
  unicorn.configs.unopinionated,

  {
    files: ["**/*.{ts,tsx,mts}"],
    languageOptions: {
      ecmaVersion: "latest",
      sourceType: "module",
      parserOptions: {
        project: true,
      },
    },
    rules: {
      "@typescript-eslint/restrict-template-expressions": [
        "error",
        {
          allowNumber: true,
          allowBoolean: true,
        },
      ],
      "unicorn/numeric-separators-style": [
        "error",
        { onlyIfContainsSeparator: true },
      ],
      "unicorn/number-literal-case": [
        "error",
        { hexadecimalValue: "lowercase" },
      ],
      "unicorn/prefer-at": "off",
      "unicorn/prefer-module": "off",
    },
  },

  {
    files: [
      "renderer-common/**/*.{ts,tsx}",
      "renderer-main/**/*.{ts,tsx}",
      "renderer-help/**/*.{ts,tsx}",
    ],
    languageOptions: { globals: globals.browser },
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
    },
  },

  {
    files: ["main/**/*.ts"],
    languageOptions: { globals: globals.node },
    rules: { "no-console": "off" },
  },

  {
    files: ["preload/**/*.ts"],
    languageOptions: { globals: { ...globals.node, ...globals.browser } },
  },
);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
