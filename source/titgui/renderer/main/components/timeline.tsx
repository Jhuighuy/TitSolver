/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import {
  IconPlayerPause,
  IconPlayerPlay,
  IconPlayerSkipBack,
  IconPlayerSkipForward,
  IconPlayerTrackNext,
  IconPlayerTrackPrev,
  IconRefresh,
  IconRepeat,
} from "@tabler/icons-react";
import { useAtom, useAtomValue } from "jotai";
import type { KeyboardEvent, PointerEvent } from "react";

import { IconButton } from "~/renderer/common/components/button";
import { chrome } from "~/renderer/common/components/classes";
import { Separator } from "~/renderer/common/components/separator";
import { Mono } from "~/renderer/common/components/text";
import { Tooltip } from "~/renderer/common/components/tooltip";
import { cn } from "~/renderer/common/components/utils";
import { useElementSize } from "~/renderer/common/hooks/use-element-size";
import { clamp } from "~/renderer/common/utils-math";
import { computeFrameTicks } from "~/renderer/main/components/timeline-scale";
import {
  isPlayingAtom,
  isRepeatingAtom,
  stopPlayback,
  togglePlayback,
} from "~/renderer/main/state/playback";
import {
  frameIndexAtom,
  frameTimeAtom,
  numFramesAtom,
  refreshStorage,
  requestFrame,
} from "~/renderer/main/state/storage";
import { assert } from "~/shared/utils";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export function Timeline() {
  // --- Playback handling. ----------------------------------------------------

  const isPlaying = useAtomValue(isPlayingAtom);
  const [isRepeating, setIsRepeating] = useAtom(isRepeatingAtom);
  const numFrames = useAtomValue(numFramesAtom);
  const frameIndex = useAtomValue(frameIndexAtom);
  const frameTime = useAtomValue(frameTimeAtom);

  function setNextFrame() {
    assert(frameIndex !== null);
    stopPlayback();
    void requestFrame(
      isRepeating
        ? (frameIndex + 1) % numFrames
        : Math.min(numFrames - 1, frameIndex + 1),
    );
  }

  function setPrevFrame() {
    assert(frameIndex !== null);
    stopPlayback();
    void requestFrame(
      isRepeating
        ? (frameIndex + numFrames - 1) % numFrames
        : Math.max(0, frameIndex - 1),
    );
  }

  // ---- Layout. --------------------------------------------------------------

  const disabled = numFrames === 0 || frameIndex === null;

  return (
    <div
      className={cn("flex h-9 shrink-0 items-center gap-1 px-2 py-1", chrome())}
    >
      {/* ---- Playback controls. ------------------------------------------ */}
      <div className="mx-2 flex items-center gap-4">
        <Tooltip content="Toggle repeat">
          <IconButton
            size="2"
            radius="full"
            active={isRepeating || undefined}
            onClick={() => {
              setIsRepeating(!isRepeating);
            }}
          >
            <IconRepeat />
          </IconButton>
        </Tooltip>

        <Separator orientation="vertical" size="2" />

        <Tooltip content="Go to start">
          <IconButton
            size="2"
            radius="full"
            disabled={disabled}
            onClick={() => {
              void requestFrame(0);
            }}
          >
            <IconPlayerTrackPrev />
          </IconButton>
        </Tooltip>

        <Separator orientation="vertical" />

        <Tooltip content="Go to previous step">
          <IconButton
            size="2"
            radius="full"
            disabled={disabled}
            onClick={setPrevFrame}
          >
            <IconPlayerSkipBack />
          </IconButton>
        </Tooltip>

        <Separator orientation="vertical" />

        <Tooltip content={isPlaying ? "Pause" : "Play"}>
          <IconButton
            size="2"
            radius="full"
            disabled={disabled}
            onClick={togglePlayback}
          >
            {isPlaying ? <IconPlayerPause /> : <IconPlayerPlay />}
          </IconButton>
        </Tooltip>

        <Separator orientation="vertical" />

        <Tooltip content="Go to next step">
          <IconButton
            size="2"
            radius="full"
            disabled={disabled}
            onClick={setNextFrame}
          >
            <IconPlayerSkipForward />
          </IconButton>
        </Tooltip>

        <Separator orientation="vertical" />

        <Tooltip content="Go to end">
          <IconButton
            size="2"
            radius="full"
            disabled={disabled}
            onClick={() => {
              void requestFrame(numFrames - 1);
            }}
          >
            <IconPlayerTrackNext />
          </IconButton>
        </Tooltip>
      </div>

      <Separator orientation="vertical" size="2" />

      <div className="mx-2 flex items-center gap-2">
        <Tooltip content="Refresh">
          <IconButton size="2" radius="full" onClick={refreshStorage}>
            <IconRefresh />
          </IconButton>
        </Tooltip>
      </div>

      <Separator orientation="vertical" size="2" />

      {/* ---- Physical time of the displayed frame. ------------------------ */}
      {frameTime !== null && (
        <>
          <Tooltip content="Physical time of the displayed frame">
            <Mono
              color="muted"
              className="mx-2 w-24 shrink-0 text-right whitespace-nowrap"
            >
              t = {formatFrameTime(frameTime)}
            </Mono>
          </Tooltip>

          <Separator orientation="vertical" size="2" />
        </>
      )}

      {/* ---- Frame scale. ------------------------------------------------- */}
      <FrameScale
        numFrames={numFrames}
        frameIndex={frameIndex}
        onScrub={(nextFrameIndex) => {
          stopPlayback();
          if (nextFrameIndex !== frameIndex) void requestFrame(nextFrameIndex);
        }}
      />
    </div>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Fixed-width time formatting, so the readout does not jitter during
// playback.
function formatFrameTime(time: number) {
  return time.toFixed(4);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

interface FrameScaleProps {
  numFrames: number;
  frameIndex: number | null;
  onScrub: (frameIndex: number) => void;
}

// Horizontal padding reserved so edge ticks and the playhead stay visible.
const SCALE_PADDING = 8;

function FrameScale({
  numFrames,
  frameIndex,
  onScrub,
}: Readonly<FrameScaleProps>) {
  const { ref, width, height } = useElementSize<HTMLDivElement>();

  const disabled = numFrames === 0 || frameIndex === null;
  const lastFrame = Math.max(numFrames - 1, 1);
  const usableWidth = Math.max(width - 2 * SCALE_PADDING, 1);

  // ---- Interaction. ----------------------------------------------------------

  function frameFromPointer(event: PointerEvent<HTMLDivElement>) {
    const rect = event.currentTarget.getBoundingClientRect();
    const t = clamp(
      (event.clientX - rect.left - SCALE_PADDING) / usableWidth,
      0,
      1,
    );
    return Math.round(t * (numFrames - 1));
  }

  function handlePointerDown(event: PointerEvent<HTMLDivElement>) {
    if (disabled || event.button !== 0) return;
    event.currentTarget.setPointerCapture(event.pointerId);
    onScrub(frameFromPointer(event));
  }

  function handlePointerMove(event: PointerEvent<HTMLDivElement>) {
    if (disabled) return;
    if (!event.currentTarget.hasPointerCapture(event.pointerId)) return;
    onScrub(frameFromPointer(event));
  }

  function handleKeyDown(event: KeyboardEvent<HTMLDivElement>) {
    if (disabled) return;
    assert(frameIndex !== null);
    switch (event.key) {
      case "ArrowLeft":
        event.preventDefault();
        onScrub(Math.max(0, frameIndex - 1));
        break;
      case "ArrowRight":
        event.preventDefault();
        onScrub(Math.min(numFrames - 1, frameIndex + 1));
        break;
      case "Home":
        event.preventDefault();
        onScrub(0);
        break;
      case "End":
        event.preventDefault();
        onScrub(numFrames - 1);
        break;
    }
  }

  // ---- Scale geometry. --------------------------------------------------------

  const frameX = (index: number) =>
    SCALE_PADDING + (numFrames > 1 ? (index / lastFrame) * usableWidth : 0);

  const pxPerFrame = numFrames > 1 ? usableWidth / lastFrame : usableWidth;
  const { minorTicks, labeledTicks } = computeFrameTicks(numFrames, pxPerFrame);

  return (
    <div
      ref={ref}
      role="slider"
      tabIndex={0}
      aria-label="Timeline"
      aria-valuemin={0}
      aria-valuemax={Math.max(numFrames - 1, 0)}
      aria-valuenow={frameIndex ?? 0}
      aria-disabled={disabled}
      onPointerDown={handlePointerDown}
      onPointerMove={handlePointerMove}
      onKeyDown={handleKeyDown}
      className="relative h-full grow cursor-pointer rounded select-none focus-visible:outline-2 focus-visible:outline-(--accent-6)"
    >
      <svg
        width="100%"
        height="100%"
        className="pointer-events-none absolute inset-0"
      >
        {/* ---- Baseline. --------------------------------------------------- */}
        <line
          x1={SCALE_PADDING}
          y1={6}
          x2={width - SCALE_PADDING}
          y2={6}
          stroke="var(--neutral-5)"
          strokeWidth={2}
        />

        {/* ---- Ticks. ------------------------------------------------------ */}
        {minorTicks.map((index) => (
          <line
            key={index}
            x1={frameX(index)}
            y1={6}
            x2={frameX(index)}
            y2={11}
            stroke="var(--neutral-5)"
            strokeWidth={1}
          />
        ))}
        {labeledTicks.map((index) => (
          <g key={index}>
            <line
              x1={frameX(index)}
              y1={6}
              x2={frameX(index)}
              y2={14}
              stroke="var(--neutral-6)"
              strokeWidth={1}
            />
            <text
              x={frameX(index)}
              y={25}
              textAnchor="middle"
              fill="var(--neutral-7)"
              fontSize="var(--text-1)"
              fontFamily="var(--font-mono)"
            >
              {index}
            </text>
          </g>
        ))}

        {/* ---- Playhead. --------------------------------------------------- */}
        {frameIndex !== null && numFrames > 0 && (
          <g fill="var(--accent-6)" stroke="var(--accent-6)">
            <polygon
              points={[
                `${frameX(frameIndex) - 4},0`,
                `${frameX(frameIndex) + 4},0`,
                `${frameX(frameIndex)},6`,
              ].join(" ")}
            />
            <line
              x1={frameX(frameIndex)}
              y1={0}
              x2={frameX(frameIndex)}
              y2={Math.max(height - 10, 14)}
              strokeWidth={2}
            />
          </g>
        )}
      </svg>
    </div>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
