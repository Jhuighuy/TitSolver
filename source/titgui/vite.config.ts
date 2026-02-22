/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/// <reference types="vitest/config" />
import { type ChildProcess, spawn } from "node:child_process";
import { type AddressInfo, createServer } from "node:net";
import path from "node:path";
import tailwindcss from "@tailwindcss/vite";
import react from "@vitejs/plugin-react";
import { defineConfig, type Plugin } from "vite";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// See https://vitejs.dev/config/ for options.
export default defineConfig({
  plugins: [
    react({
      babel: {
        plugins: [["babel-plugin-react-compiler"]],
      },
    }),
    tailwindcss(),
    titapp(),
  ],
  base: "./",
  resolve: {
    alias: {
      "~": path.resolve(__dirname, "./src"),
    },
  },
  build: {
    outDir: process.env.PNPM_OUTPUT_DIR ?? "dist",
    emptyOutDir: true,
  },
  test: {
    globals: true,
    environment: "jsdom",
    coverage: {
      reporter: ["lcov"],
    },
  },
});

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Vite plugin to run the application server alongside the development server.
function titapp(): Plugin {
  let backend: ChildProcess | null = null;
  let backendPort: number | null = null;

  async function startBackend() {
    if (backend !== null || backendPort !== null) {
      throw new Error("Backend already running.");
    }

    // Find a free port for the backend to listen on.
    backendPort = await findFreePort();

    // Spawn the backend and wait for the it to start.
    backend = spawn(
      "./output/TIT_ROOT/bin/titapp",
      ["--headless", "--port", backendPort.toString()],
      { cwd: "../../" },
    );
    await new Promise<void>((resolve, reject) => {
      if (backend === null) return reject(new Error("Backend not spawned."));
      const timeout = setTimeout(
        () => reject(new Error("Backend startup timeout.")),
        5000,
      );
      backend.on("error", (err) => {
        clearTimeout(timeout);
        reject(err);
      });
      backend.on("close", (code) => {
        if (code !== 0) {
          clearTimeout(timeout);
          reject(new Error(`Backend exited with code ${code}`));
        }
      });
      backend.stdout?.on("data", (data: Buffer) => {
        const text = data.toString();
        console.log(text);
      });
      backend.stderr?.on("data", (data: Buffer) => {
        const text = data.toString();
        console.error(text);
        if (text.includes("running")) {
          clearTimeout(timeout);
          resolve();
        }
      });
    });

    console.info(`titapp started [${backend.pid}].`);
  }

  function cleanupBackend() {
    if (!backend || backend.killed) return;
    backend.kill("SIGTERM");
    console.info(`titapp closed [${backend.pid}].`);
  }

  return {
    name: "run-titapp",
    async config(_, { command }) {
      if (command !== "serve" && process.env.VITE_TEST !== "true") {
        return;
      }
      await startBackend();
      process.on("exit", cleanupBackend);
      process.on("SIGINT", cleanupBackend);
      process.on("SIGTERM", cleanupBackend);
      return {
        server: {
          proxy: {
            "/ws": {
              target: `ws://localhost:${backendPort}`,
              ws: true,
              changeOrigin: true,
            },
            "/manual": {
              target: `http://localhost:${backendPort}`,
              changeOrigin: true,
            },
            "/export": {
              target: `http://localhost:${backendPort}`,
              changeOrigin: true,
            },
          },
        },
      };
    },
    configureServer(server) {
      server.httpServer?.on("close", cleanupBackend);
    },
    closeBundle() {
      cleanupBackend();
    },
  };
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

async function findFreePort(): Promise<number> {
  return new Promise<number>((resolve) => {
    const server = createServer();
    server.listen(0, () => {
      const port = (server.address() as AddressInfo).port;
      server.close(() => resolve(port));
    });
  });
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
