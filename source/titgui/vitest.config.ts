/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import path from "node:path";

import { defineConfig } from "vitest/config";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export default defineConfig({
  // Build-time constants injected by Electron Forge's vite plugin.
  define: {
    RENDERER_VITE_DEV_SERVER_URL: "undefined",
    RENDERER_VITE_NAME: '"renderer"',
  },
  resolve: {
    alias: [
      // The svgr plugin does not run under vitest; stub `*.svg?react`.
      {
        find: /^~\/assets\/.+\.svg\?react$/u,
        replacement: path.resolve(__dirname, "renderer/common/svg-stub.tsx"),
      },
      { find: "~", replacement: __dirname },
    ],
  },
  test: {
    environment: "node",
    include: ["**/*.test.{ts,tsx}"],
    exclude: [
      "**/node_modules/**",
      ".vite/**",
      "dist/**",
      "e2e/**",
      "native/**",
    ],
    setupFiles: ["./vitest.setup.ts"],
    coverage: {
      provider: "v8",
      reporter: ["text-summary", "lcov"],
      include: [
        "bindings/**/*.ts",
        "main/**",
        "preload/**",
        "renderer/**",
        "shared/**",
      ],
    },
  },
});

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
