/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { z } from "zod";
import { create } from "zustand";
import { useQuery, useQueryClient } from "@tanstack/react-query";

import { useConnection } from "~/hooks/use-connection";
import { assert, decodeBase64, minMax } from "~/utils";
import type { FieldMap } from "~/visual/particles";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export type Storage = {
  numFrames: number;
  isFrameLoading: boolean;
  frameIndex: number | null;
  frameData: FieldMap | null;
  requestFrame: (frameIndex: number) => void;
  refresh: () => void;
};

export function useStorage(): Storage {
  const { frameIndex, setFrameIndex } = useStorageStore();

  const { sendMessageAsync } = useConnection();
  const queryClient = useQueryClient();

  // ---- Number of frames. ----------------------------------------------------

  const numFramesQuery = useQuery({
    queryKey: ["storage", "numFrames"],
    queryFn: async () => {
      const responseRaw = await sendMessageAsync({ type: "num-frames" });
      const numFrames = numFramesSchema.parse(responseRaw);
      if (numFrames > 0) setFrameIndex(0);
      return numFrames;
    },
  });

  const numFrames = numFramesQuery.data ?? 0;

  // ---- Frame data. ----------------------------------------------------------

  const frameDataQuery = useQuery({
    enabled: frameIndex !== null && numFramesQuery.isSuccess,
    queryKey: ["storage", "frameData", frameIndex],
    queryFn: async () => {
      assert(frameIndex !== null);
      assert(frameIndex >= 0 && frameIndex < numFrames);

      const responseRaw = await sendMessageAsync({
        type: "frame",
        index: frameIndex,
      });

      const frameDataRaw = frameDataSchema.parse(responseRaw);
      const frameData: FieldMap = {};
      for (const [fieldName, field] of Object.entries(frameDataRaw)) {
        const { kind, data } = field;
        const vals = decodeBase64(data, kind);
        const [min, max] = minMax(vals);
        frameData[fieldName] = { min, max, data: vals };
      }

      return frameData;
    },
  });

  const isFrameLoading = frameDataQuery.isLoading;
  const frameData = frameDataQuery.data ?? null;

  const requestFrame = (index: number) => {
    if (numFrames > 0 && index >= 0 && index < numFrames) {
      setFrameIndex(index);
    } else {
      console.warn(`Attempted to request invalid frame index: ${index}`);
    }
  };

  const refresh = () => {
    setFrameIndex(null);
    void queryClient.resetQueries({ queryKey: ["storage"] });
  };

  return {
    numFrames,
    isFrameLoading,
    frameIndex,
    frameData,
    requestFrame,
    refresh,
  };
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

type StorageStore = {
  frameIndex: number | null;
  setFrameIndex: (index: number | null) => void;
};

const useStorageStore = create<StorageStore>((set) => ({
  frameIndex: null,
  setFrameIndex: (index) => set({ frameIndex: index }),
}));

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

const numFramesSchema = z.number().min(0);

const frameDataSchema = z.record(
  z.string(),
  z.object({
    kind: z.string(),
    data: z.string(),
  })
);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
