/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { ChildProcess, spawn } from "child_process";
import path from "path";
import "whatwg-fetch"; // Import the fetch

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

let serverProcess: ChildProcess | null = null;

beforeAll((done) => {
  // Start the backend server.
  serverProcess = spawn(
    path.join(__dirname, "../output/TIT_ROOT/bin/tit_server")
  );
  serverProcess.stdout.on("data", (data) => {
    console.log(`stdout: ${data}`);
    if (data.includes("Server is running")) {
      done();
    }
  });
});

afterAll(() => {
  // Ensure the server is killed when Jest quits.
  if (serverProcess) {
    serverProcess.kill();
  }
});

// Mock window.electron object
global.window = Object.create(window);
Object.defineProperty(window, "electron", {
  value: {
    fetchFromServer: async (endpoint: string): Promise<string> => {
      console.log(`fetchFromServer(${endpoint})`);
      const url = "http://localhost:18080";
      const response = await fetch(`${url}${endpoint}`);
      return response.text();
    },
  },
});

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
