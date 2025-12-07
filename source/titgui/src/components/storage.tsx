/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import {
  createContext,
  type ReactNode,
  useContext,
  useEffect,
  useEffectEvent,
  useMemo,
  useRef,
  useState,
} from "react";
import { z } from "zod";

import { useConnection } from "~/components/connection";
import { assert, decodeBase64, minMax } from "~/utils";
import type { FieldMap } from "~/visual/particles";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export type Storage = {
  numFrames: number;
  frameIndex: number | null;
  frameData: FieldMap | null;
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

export function StorageProvider({ children }: Readonly<StorageProviderProps>) {
  const [numFrames, setNumFrames] = useState<number | null>(null);
  const [requestedFrameIndex, setRequestedFrameIndex] = useState<number | null>(
    null
  );
  const [frameIndex, setFrameIndex] = useState<number | null>(null);
  const [frameData, setFrameData] = useState<FieldMap | null>(null);

  const { sendMessage } = useConnection();

  // ---- Number of frames. ----------------------------------------------------

  const sessionIDRef = useRef(0);

  const requestNumFramesEvent = useEffectEvent(() => {
    const sessionID = ++sessionIDRef.current;

    sendMessage({ type: "num-frames" }, (result) => {
      if (sessionID !== sessionIDRef.current) return;

      const numFrames = numFramesSchema.parse(result);
      assert(numFrames >= 0);
      setNumFrames(numFrames);

      if (numFrames > 0) setRequestedFrameIndex(0);
    });
  });

  useEffect(() => {
    if (numFrames === null) requestNumFramesEvent();
  }, [numFrames]);

  // ---- Current frame. -------------------------------------------------------

  const requestIDRef = useRef(0);

  const requestFrameEvent = useEffectEvent(() => {
    assert(numFrames !== null && requestedFrameIndex !== null);
    assert(0 <= requestedFrameIndex && requestedFrameIndex < numFrames);

    const sessionID = sessionIDRef.current;
    const requestID = ++requestIDRef.current;

    sendMessage({ type: "frame", index: requestedFrameIndex }, (result) => {
      if (sessionID !== sessionIDRef.current) return;
      if (requestID !== requestIDRef.current) return;

      const rawFrameData = frameDataSchema.parse(result);

      const frameData: FieldMap = {};
      for (const [fieldName, field] of Object.entries(rawFrameData)) {
        const { kind, data } = field;
        const vals = decodeBase64(data, kind);
        const [min, max] = minMax(vals);
        frameData[fieldName] = { min, max, data: vals };
      }

      setFrameData(frameData);
      setFrameIndex(requestedFrameIndex);

      setRequestedFrameIndex(null);
    });
  });

  useEffect(() => {
    if (requestedFrameIndex !== null) requestFrameEvent();
  }, [requestedFrameIndex]);

  // ---- Context. -------------------------------------------------------------

  const storage = useMemo(
    () => ({
      numFrames: numFrames ?? 0,
      frameIndex,
      frameData,
      requestFrame(frameIndex: number) {
        assert(numFrames !== null);
        assert(0 <= frameIndex && frameIndex < numFrames);
        setRequestedFrameIndex(frameIndex);
      },
      refresh() {
        setNumFrames(null);
        setRequestedFrameIndex(null);
        setFrameIndex(null);
        setFrameData(null);
      },
    }),
    [numFrames, frameIndex, frameData]
  );

  // ---- Layout. --------------------------------------------------------------

  return (
    <StorageContext.Provider value={storage}>
      {children}
    </StorageContext.Provider>
  );
}

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
