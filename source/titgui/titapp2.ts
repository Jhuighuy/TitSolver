import { spawn } from "node:child_process";
import type { AddressInfo } from "node:net";
import type { Plugin } from "vite";

async function startBackend(): Promise<{
  port: number;
  process: ReturnType<typeof spawn>;
}> {
  const port = await randomFreePort();
  process.env.TIT_BACKEND_PORT = port.toString();
  const proc = spawn("./output/TIT_ROOT/bin/titback", {
    cwd: "../../",
  });

  await new Promise<void>((resolve, reject) => {
    const timeout = setTimeout(
      () => reject(new Error("Backend startup timeout")),
      5000
    );

    proc.stdout?.on("data", (d: Buffer) => {
      const text = d.toString();
      console.log(text);
    });

    proc.stderr?.on("data", (d: Buffer) => {
      const text = d.toString();
      console.error(text);
      if (text.includes("running")) {
        clearTimeout(timeout);
        resolve();
      }
    });
  });

  console.log(`titapp started on port ${port} [pid=${proc.pid}]`);

  return { port, process: proc };
}

export function titapp2(): Plugin {
  let cleanup: (() => void) | null = null;

  return {
    name: "titapp-backend",

    async config() {
      // At config time we start the backend and inject Vite's proxy config.
      const { port, process } = await startBackend();

      cleanup = () => {
        process.kill();
        console.log(`titapp stopped [pid=${process.pid}]`);
      };

      return {
        server: {
          proxy: {
            // Proxy API calls → backend
            "/api": {
              target: `http://localhost:${port}`,
              changeOrigin: true,
            },
            // Proxy WebSocket → backend
            "/ws": {
              target: `ws://localhost:${port}`,
              ws: true,
              changeOrigin: true,
            },
          },
        },
      };
    },

    closeBundle() {
      // Vite build exit cleanup
      if (cleanup) cleanup();
    },

    configureServer(server) {
      // Dev server exit cleanup
      server.httpServer?.on("close", () => {
        if (cleanup) cleanup();
      });
    },
  };
}

// Helper ---------------------------------------------------------
async function randomFreePort(): Promise<number> {
  const net = await import("node:net");
  return await new Promise((resolve) => {
    const srv = net.createServer();
    srv.listen(0, () => {
      const port = (srv.address() as AddressInfo).port;
      srv.close(() => resolve(port));
    });
  });
}
