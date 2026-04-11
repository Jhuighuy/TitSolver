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

import { FieldMap } from "~/renderer-common/visual/fields";
import { assert } from "~/shared/utils";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export interface Storage {
  numFrames: number;
  frameIndex: number | null;
  frameData: FieldMap;
  requestFrame: (frameIndex: number) => void;
  refresh: () => void;
}

const StorageContext = createContext<Storage | null>(null);

export function useStorage(): Storage {
  const context = useContext(StorageContext);
  assert(context !== null, "Storage is not available.");
  return context;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

interface StorageProviderProps {
  children: ReactNode;
}

export function StorageProvider({ children }: Readonly<StorageProviderProps>) {
  const [numFrames, setNumFrames] = useState<number | null>(null);
  const [requestedFrameIndex, setRequestedFrameIndex] = useState<number | null>(
    null,
  );
  const [frameIndex, setFrameIndex] = useState<number | null>(null);
  const [frameData, setFrameData] = useState(new FieldMap({}));

  // ---- Number of frames. ----------------------------------------------------

  const sessionIDRef = useRef(0);

  const requestNumFramesEvent = useEffectEvent(() => {
    const sessionID = ++sessionIDRef.current;

    void globalThis.session?.getFrameCount().then((numFrames) => {
      if (sessionID !== sessionIDRef.current) return;

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

    void globalThis.session?.getFrame(requestedFrameIndex).then((result) => {
      if (sessionID !== sessionIDRef.current) return;
      if (requestID !== requestIDRef.current) return;

      const fieldMap = new FieldMap(
        Object.fromEntries(
          Object.entries(result).map(([fieldName, { data }]) => [
            fieldName,
            data,
          ]),
        ),
      );

      setFrameData(fieldMap);
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
        setFrameData(new FieldMap({}));
      },
    }),
    [numFrames, frameIndex, frameData],
  );

  // ---- Layout. --------------------------------------------------------------

  return (
    <StorageContext.Provider value={storage}>
      {children}
    </StorageContext.Provider>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
