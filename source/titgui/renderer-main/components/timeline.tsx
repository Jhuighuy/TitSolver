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
import { useEffect, useEffectEvent, useState } from "react";
import ScrollContainer from "react-indiana-drag-scroll";

import { IconButton } from "~/renderer-common/components/button";
import { chrome, hoverSurface } from "~/renderer-common/components/classes";
import { Box, Flex } from "~/renderer-common/components/layout";
import { Separator } from "~/renderer-common/components/separator";
import { Text } from "~/renderer-common/components/text";
import { Tooltip } from "~/renderer-common/components/tooltip";
import { useStorage } from "~/renderer-main/components/storage";
import { assert, iota } from "~/shared/utils";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export function Timeline() {
  // --- Playback handling. ----------------------------------------------------

  const [isPlaying, setIsPlaying] = useState(false);
  const [isRepeating, setIsRepeating] = useState(false);
  const { numFrames, frameIndex, requestFrame, refresh } = useStorage();

  function setNextFrame() {
    assert(frameIndex !== null);
    setIsPlaying(false);
    requestFrame(
      isRepeating
        ? (frameIndex + 1) % numFrames
        : Math.min(numFrames - 1, frameIndex + 1),
    );
  }

  function setPrevFrame() {
    assert(frameIndex !== null);
    setIsPlaying(false);
    requestFrame(
      isRepeating
        ? (frameIndex + numFrames - 1) % numFrames
        : Math.max(0, frameIndex - 1),
    );
  }

  function clickOnFrame(frameIndex: number) {
    setIsPlaying(false);
    requestFrame(frameIndex);
  }

  const playNextFrameEvent = useEffectEvent(() => {
    if (frameIndex === null) return;
    if (!isRepeating && frameIndex === numFrames - 1) {
      setIsPlaying(false);
      return;
    }
    const nextFrameIndex = (frameIndex + 1) % numFrames;
    requestFrame(nextFrameIndex);
  });

  useEffect(() => {
    if (isPlaying) {
      const timeout = setInterval(playNextFrameEvent, 1000 / 60);
      return () => {
        clearInterval(timeout);
      };
    }
  }, [isPlaying]);

  // ---- Layout. --------------------------------------------------------------

  const alignedNumFrames =
    10 ** Math.ceil(Math.log10(Math.max(numFrames, 1))) + 1;

  return (
    <Flex
      align="center"
      height="9"
      minHeight="9"
      maxHeight="9"
      px="2"
      py="1"
      gap="1"
      className={chrome()}
    >
      {/* ---- Playback controls. ------------------------------------------ */}
      <Flex align="center" gap="4" mx="2">
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
            disabled={numFrames === 0 || frameIndex === null}
            onClick={() => {
              requestFrame(0);
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
            disabled={numFrames === 0 || frameIndex === null}
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
            disabled={numFrames === 0 || frameIndex === null}
            onClick={() => {
              setIsPlaying((x) => !x);
            }}
          >
            {isPlaying ? <IconPlayerPause /> : <IconPlayerPlay />}
          </IconButton>
        </Tooltip>

        <Separator orientation="vertical" />

        <Tooltip content="Go to next step">
          <IconButton
            size="2"
            radius="full"
            disabled={numFrames === 0 || frameIndex === null}
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
            disabled={numFrames === 0 || frameIndex === null}
            onClick={() => {
              requestFrame(numFrames - 1);
            }}
          >
            <IconPlayerTrackNext />
          </IconButton>
        </Tooltip>
      </Flex>

      <Separator orientation="vertical" size="2" />

      <Flex align="center" gap="2" mx="2">
        <Tooltip content="Refresh">
          <IconButton size="2" radius="full" onClick={refresh}>
            <IconRefresh />
          </IconButton>
        </Tooltip>
      </Flex>

      <Separator orientation="vertical" size="2" />

      {/* ---- Time steps. ------------------------------------------------- */}
      <Box flexGrow="1" size="100%">
        <ScrollContainer vertical={false} className="h-full w-full">
          <Box position="relative" height="100%">
            {iota(alignedNumFrames).map((tickIndex) => {
              const inRange = tickIndex < numFrames;
              const isLarge = tickIndex % 10 === 0;
              const sizeMultiplier = isLarge ? 2 : 1;

              return (
                <Box
                  key={tickIndex}
                  position="absolute"
                  left={`calc(10px * ${tickIndex})`}
                  top="0"
                  width="10px"
                  height="100%"
                  {...(inRange && {
                    onClick: () => {
                      clickOnFrame(tickIndex);
                    },
                    className: `rounded ${hoverSurface()}`,
                  })}
                >
                  {/* ---- Horizontal bar. --------------------------------- */}
                  <Box
                    position="absolute"
                    left={0 < tickIndex ? "0" : "50%"}
                    top="10%"
                    width={
                      tickIndex === 0 || tickIndex === alignedNumFrames - 1
                        ? "50%"
                        : "100%"
                    }
                    height="2px"
                    className={inRange ? "bg-(--fg-3)" : "bg-(--fg-5)"}
                  />

                  {/* ---- Vertical bar. ----------------------------------- */}
                  <Box
                    position="absolute"
                    top="10%"
                    left="50%"
                    width={`calc(${sizeMultiplier} * 1px)`}
                    height={`calc(${sizeMultiplier} * 20%)`}
                    className={inRange ? "bg-(--fg-3)" : "bg-(--fg-5)"}
                    style={{
                      transform: "translateX(-50%)",
                    }}
                  />

                  {/* ---- Time step index. -------------------------------- */}
                  {isLarge && (
                    <Box
                      position="absolute"
                      left="50%"
                      top="40%"
                      style={{ zIndex: 10, transform: "translateX(-50%)" }}
                    >
                      <Text
                        mono
                        className={inRange ? "text-(--fg-3)" : "text-(--fg-5)"}
                      >
                        {tickIndex}
                      </Text>
                    </Box>
                  )}

                  {/* ---- Pointer. ---------------------------------------- */}
                  {tickIndex === frameIndex && (
                    <Box
                      position="absolute"
                      size="10px"
                      className="bg-(--accent-bg-3)"
                      style={{
                        borderTopLeftRadius: "0.25rem",
                        borderTopRightRadius: "0.25rem",
                      }}
                    >
                      <Box
                        position="absolute"
                        left="50%"
                        size={`calc(100% / ${Math.SQRT2})`}
                        className="bg-(--accent-bg-3)"
                        style={{
                          transform: "translate(-50%, 100%) rotate(45deg)",
                        }}
                      />
                    </Box>
                  )}
                </Box>
              );
            })}
          </Box>
        </ScrollContainer>
      </Box>
    </Flex>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
