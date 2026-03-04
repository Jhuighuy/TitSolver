/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { z } from "zod";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

const backendBaseUrl = "http://localhost:18080";
export const backendUrl = (path: string) => `${backendBaseUrl}${path}`;

const responseSchema = z.discriminatedUnion("status", [
  z.object({
    status: z.literal("success"),
    result: z.unknown().optional(),
  }),
  z.object({
    status: z.literal("error"),
    error: z.string(),
  }),
]);

async function callBackend(path: string, init?: RequestInit): Promise<unknown> {
  const headers = new Headers(init?.headers);
  if (!headers.has("Accept")) headers.set("Accept", "application/json");
  if (
    init?.body !== undefined &&
    init.body !== null &&
    !headers.has("Content-Type")
  ) {
    headers.set("Content-Type", "application/json");
  }

  const response = await fetch(backendUrl(path), {
    ...init,
    headers,
  });

  if (!response.ok) {
    throw new Error(`Backend request failed: ${response.status}`);
  }

  const payload = responseSchema.parse(await response.json());
  if (payload.status === "error") throw new Error(payload.error);
  return payload.result;
}

export async function getNumFrames() {
  return z
    .number()
    .int()
    .min(0)
    .parse(await callBackend("/api/storage/num-frames"));
}

const rawFrameSchema = z.record(
  z.string(),
  z.object({
    kind: z.string(),
    data: z.string(),
  }),
);

export type RawFrameData = z.infer<typeof rawFrameSchema>;

export async function getFrame(frameIndex: number): Promise<RawFrameData> {
  return rawFrameSchema.parse(
    await callBackend(`/api/storage/frame/${frameIndex.toString()}`),
  );
}

const solverStatusSchema = z.object({
  isRunning: z.boolean(),
  output: z.string(),
});

export type SolverStatus = z.infer<typeof solverStatusSchema>;

export async function getSolverStatus(): Promise<SolverStatus> {
  return solverStatusSchema.parse(await callBackend("/api/solver/status"));
}

export async function runSolver() {
  await callBackend("/api/solver/run", { method: "POST" });
}

export async function stopSolver() {
  await callBackend("/api/solver/stop", { method: "POST" });
}

export async function exportStorage() {
  return z.string().parse(await callBackend("/api/export", { method: "POST" }));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
