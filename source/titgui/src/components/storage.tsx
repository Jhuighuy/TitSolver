/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { useQuery, useQueryClient } from "@tanstack/react-query";
import {
  createContext,
  type ReactNode,
  useContext,
  useEffect,
  useMemo,
  useRef,
  useState,
  useSyncExternalStore,
} from "react";

import { getFrame, getNumFrames } from "~/backend-api";
import { assert, decodeBase64, minMax } from "~/utils";
import type { FieldMap } from "~/visual/particles";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export type Storage = {
  numFrames: number;
  frameIndex: number | null;
  frameData: FieldMap | null;
  loadedFrames: number[];
  requestFrame: (frameIndex: number) => void;
  refresh: () => void;
};

const StorageContext = createContext<Storage | null>(null);

export function useStorage(): Storage {
  const context = useContext(StorageContext);
  assert(context !== null, "Storage is not available.");
  return context;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

type StorageProviderProps = {
  children: ReactNode;
};

function numFramesQueryKey(sessionID: number) {
  return ["storage", "num-frames", sessionID] as const;
}

function frameQueryKey(sessionID: number, frameIndex: number) {
  return ["storage", "frame", sessionID, frameIndex] as const;
}

export function StorageProvider({ children }: Readonly<StorageProviderProps>) {
  const preloadWindow = 8;
  const keepBehindWindow = 4;
  const keepAheadWindow = 16;
  const queryClient = useQueryClient();
  const [sessionID, setSessionID] = useState(0);
  const loadedFramesSnapshotRef = useRef<number[]>([]);
  const [requestedFrameIndex, setRequestedFrameIndex] = useState<number | null>(
    null,
  );

  const numFramesQuery = useQuery({
    queryKey: numFramesQueryKey(sessionID),
    queryFn: getNumFrames,
  });

  const numFrames = numFramesQuery.data ?? 0;
  const frameIndex =
    numFrames === 0
      ? null
      : requestedFrameIndex === null || requestedFrameIndex >= numFrames
        ? 0
        : requestedFrameIndex;

  const frameQuery = useQuery({
    queryKey:
      frameIndex !== null
        ? frameQueryKey(sessionID, frameIndex)
        : frameQueryKey(sessionID, -1),
    queryFn: () => {
      assert(frameIndex !== null);
      return getFrame(frameIndex);
    },
    enabled: frameIndex !== null && numFrames > 0,
    staleTime: 30_000,
  });

  useEffect(() => {
    if (frameIndex === null) return;

    const minFrameIndex = Math.max(0, frameIndex - keepBehindWindow);
    const maxFrameIndex = Math.min(numFrames - 1, frameIndex + keepAheadWindow);

    queryClient.removeQueries({
      predicate: (query) => {
        const [scope, type, querySessionID, queryFrameIndex] = query.queryKey;
        if (
          scope !== "storage" ||
          type !== "frame" ||
          querySessionID !== sessionID ||
          typeof queryFrameIndex !== "number"
        ) {
          return false;
        }
        return queryFrameIndex < minFrameIndex || maxFrameIndex < queryFrameIndex;
      },
    });

    for (let offset = 1; offset <= preloadWindow; ++offset) {
      const nextFrameIndex = frameIndex + offset;
      if (nextFrameIndex > maxFrameIndex) break;

      void queryClient.prefetchQuery({
        queryKey: frameQueryKey(sessionID, nextFrameIndex),
        queryFn: () => getFrame(nextFrameIndex),
        staleTime: 30_000,
      });
    }
  }, [frameIndex, numFrames, queryClient, sessionID]);

  const loadedFrames = useSyncExternalStore(
    (onStoreChange) => queryClient.getQueryCache().subscribe(onStoreChange),
    () => {
      const nextLoaded = queryClient
        .getQueryCache()
        .findAll({ queryKey: ["storage", "frame", sessionID] })
        .flatMap((query) => {
          const keyPart = query.queryKey[3];
          if (typeof keyPart !== "number") return [];
          if (query.state.data === undefined) return [];
          if (keyPart < 0 || keyPart >= numFrames) return [];
          return [keyPart];
        });

      const dedupedSorted = [...new Set(nextLoaded)].sort((a, b) => a - b);
      const prev = loadedFramesSnapshotRef.current;
      const isSame =
        prev.length === dedupedSorted.length &&
        prev.every((value, index) => value === dedupedSorted[index]);
      if (isSame) return prev;

      loadedFramesSnapshotRef.current = dedupedSorted;
      return dedupedSorted;
    },
    () => [],
  );

  const frameData = useMemo<FieldMap | null>(() => {
    if (frameQuery.data === undefined) return null;

    const decodedFrameData: FieldMap = {};
    for (const [fieldName, field] of Object.entries(frameQuery.data)) {
      const vals = decodeBase64(field.data, field.kind);
      const [min, max] = minMax(vals);
      decodedFrameData[fieldName] = { min, max, data: vals };
    }
    return decodedFrameData;
  }, [frameQuery.data]);

  const storage = useMemo<Storage>(
    () => ({
      numFrames,
      frameIndex,
      frameData,
      loadedFrames,
      requestFrame(nextFrameIndex: number) {
        assert(0 <= nextFrameIndex && nextFrameIndex < numFrames);
        setRequestedFrameIndex(nextFrameIndex);
      },
      refresh() {
        setRequestedFrameIndex(null);
        loadedFramesSnapshotRef.current = [];
        setSessionID((prev) => prev + 1);
        void queryClient.removeQueries({ queryKey: ["storage", "frame"] });
      },
    }),
    [frameData, frameIndex, loadedFrames, numFrames, queryClient],
  );

  return (
    <StorageContext.Provider value={storage}>
      {children}
    </StorageContext.Provider>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
