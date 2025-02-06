/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { spawn } from "node:child_process";
import { createServer } from "node:http";
import httpProxy from "http-proxy";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
 * Backend process options.
 */
interface BackendOptions {
  /**
   * Port of the backend process.
   * @default randomFreePort()
   */
  backendPort?: number;

  /**
   * Port of the proxy HTTP server.
   * @default randomFreePort()
   */
  proxyPort?: number;
}

/**
 * Backend process and proxy server (actual) configuration.
 */
interface BackendConfig {
  /** Port of the backend process. */
  backendPort: number;

  /** Port of the proxy HTTP server. */
  proxyPort: number;

  /** Cleanup function to stop the backend process and proxy server. */
  cleanup: () => void;
}

/**
 * Setup the backend process and remap it to a proxy server.
 */
export async function setupBackend(
  options: BackendOptions = {}
): Promise<BackendConfig> {
  const backendPort = options.backendPort ?? (await randomFreePort());
  process.env.TIT_BACKEND_PORT = backendPort.toString();
  const backendProcess = spawn("./output/TIT_ROOT/bin/titback", {
    cwd: "../../",
  });
  backendProcess.on("exit", (code, signal) => {
    if (code === 0) return;
    console.error(`titback exited with code ${code} / signal ${signal}`);
  });
  await new Promise<void>((resolve) => {
    backendProcess.stderr?.on("data", (data: Buffer) => {
      if (data.toString().includes("running")) resolve();
    });
  });

  const proxySpec = {
    target: `http://localhost:${backendPort}`,
    changeOrigin: true,
    ws: true,
  };
  const proxyServer = createServer((request, response) => {
    const proxy = httpProxy.createProxyServer(proxySpec);
    proxy.on("error", (error) => {
      console.error("Proxy error:", error.message);
      response.writeHead(502);
      response.end("Proxy error.");
    });
    proxy.web(request, response);
  });
  proxyServer.on("upgrade", (request, socket, head) => {
    const proxy = httpProxy.createProxyServer(proxySpec);
    proxy.on("error", (error) => {
      console.error("Proxy error:", error.message);
      socket.end("Proxy error.");
    });
    proxy.ws(request, socket, head);
  });
  const proxyPort = options.proxyPort ?? (await randomFreePort());
  proxyServer.listen(proxyPort, () => {});
  console.info(`titback started [${backendProcess.pid}].`);

  return {
    backendPort,
    proxyPort,
    cleanup() {
      proxyServer.close();
      backendProcess.kill();
      console.info(`titback closed [${backendProcess.pid}].`);
    },
  };
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Find a random free port.
async function randomFreePort() {
  const minPort = 1025;
  const maxPort = 65535;
  return new Promise<number>((resolve, reject) => {
    const tryPort = () => {
      // Square root would make a tendency to pick larger ports, those ports are
      // more likely to be free.
      const port = Math.floor(
        minPort + Math.sqrt(Math.random()) * (maxPort - minPort + 1)
      );
      const server = createServer();
      server.on("error", (error: NodeJS.ErrnoException) => {
        if (error.code === "EADDRINUSE") tryPort();
        else reject(error);
      });
      const onClose = () => resolve(port);
      server.listen(port, () => server.close(onClose));
    };
    tryPort();
  });
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
