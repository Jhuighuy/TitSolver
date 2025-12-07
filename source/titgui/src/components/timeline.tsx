/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { Box, Flex, IconButton, Separator, Tooltip } from "@radix-ui/themes";
import { useEffect, useEffectEvent, useState } from "react";
import {
  FaBackward as BackIcon,
  FaForward as ForwardIcon,
  FaPause as PauseIcon,
  FaPlay as PlayIcon,
  FaStepBackward as StepBackwardIcon,
  FaStepForward as StepForwardIcon,
} from "react-icons/fa";
import {
  FaArrowsRotate as RefreshIcon,
  FaRepeat as RepeatIcon,
} from "react-icons/fa6";
import ScrollContainer from "react-indiana-drag-scroll";

import { TechText } from "~/components/basic";
import { useStorage } from "~/components/storage";
import { assert, iota } from "~/utils";

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
        : Math.min(numFrames - 1, frameIndex + 1)
    );
  }

  function setPrevFrame() {
    assert(frameIndex !== null);
    setIsPlaying(false);
    requestFrame(
      isRepeating
        ? (frameIndex + numFrames - 1) % numFrames
        : Math.max(0, frameIndex - 1)
    );
  }

  function clickOnFrame(frameIndex: number) {
    setIsPlaying(false);
    requestFrame(frameIndex);
  }

  const playNextFrameEvent = useEffectEvent(() => {
    assert(frameIndex !== null);
    if (!isRepeating && frameIndex === numFrames - 1) {
      setIsPlaying(false);
      return;
    }
    const nextFrameIndex = (frameIndex + 1) % numFrames;
    requestFrame(nextFrameIndex);
  });

  useEffect(() => {
    if (frameIndex === null) return;
    if (isPlaying) playNextFrameEvent();
  }, [isPlaying, frameIndex]);

  // ---- Layout. --------------------------------------------------------------

  const iconSize = 16;
  const alignedNumFrames =
    10 ** Math.ceil(Math.log10(Math.max(numFrames, 1))) + 1;

  return (
    <Flex
      direction="row"
      align="center"
      height="36px"
      px="2"
      py="4px"
      gap="1"
      className="bg-linear-to-br from-gray-700 to-gray-800 inset-shadow-sm inset-shadow-gray-700"
    >
      {/* ---- Playback controls. ------------------------------------------ */}
      <Flex direction="row" align="center" gap="4" mx="2">
        <Tooltip content="Toggle repeat">
          <IconButton
            variant="ghost"
            color={isRepeating ? "blue" : "gray"}
            radius="full"
            onClick={() => setIsRepeating(!isRepeating)}
          >
            <RepeatIcon size={iconSize} />
          </IconButton>
        </Tooltip>

        <Separator orientation="vertical" size="1" />

        <Tooltip content="Go to start">
          <IconButton
            variant="ghost"
            color="gray"
            radius="full"
            disabled={numFrames === 0 || frameIndex === null}
            onClick={() => requestFrame(0)}
          >
            <BackIcon size={iconSize} />
          </IconButton>
        </Tooltip>

        <Separator orientation="vertical" size="1" />

        <Tooltip content="Go to previous step">
          <IconButton
            variant="ghost"
            color="gray"
            radius="full"
            disabled={numFrames === 0 || frameIndex === null}
            onClick={setPrevFrame}
          >
            <StepBackwardIcon size={iconSize} />
          </IconButton>
        </Tooltip>

        <Separator orientation="vertical" size="1" />

        <Tooltip content={isPlaying ? "Pause" : "Play"}>
          <IconButton
            variant="ghost"
            color="gray"
            radius="full"
            disabled={numFrames === 0 || frameIndex === null}
            onClick={() => setIsPlaying((x) => !x)}
          >
            {isPlaying ? (
              <PauseIcon size={iconSize} />
            ) : (
              <PlayIcon size={iconSize} />
            )}
          </IconButton>
        </Tooltip>

        <Separator orientation="vertical" size="1" />

        <Tooltip content="Go to next step">
          <IconButton
            variant="ghost"
            color="gray"
            radius="full"
            disabled={numFrames === 0 || frameIndex === null}
            onClick={setNextFrame}
          >
            <StepForwardIcon size={iconSize} />
          </IconButton>
        </Tooltip>

        <Separator orientation="vertical" size="1" />

        <Tooltip content="Go to end">
          <IconButton
            variant="ghost"
            color="gray"
            radius="full"
            disabled={numFrames === 0 || frameIndex === null}
            onClick={() => requestFrame(numFrames - 1)}
          >
            <ForwardIcon size={iconSize} />
          </IconButton>
        </Tooltip>
      </Flex>

      <Separator orientation="vertical" size="4" />

      <Flex direction="row" align="center" gap="2" mx="2">
        <Tooltip content="Refresh">
          <IconButton
            variant="ghost"
            color="gray"
            radius="full"
            onClick={refresh}
          >
            <RefreshIcon size={iconSize} />
          </IconButton>
        </Tooltip>
      </Flex>

      <Separator orientation="vertical" size="4" />

      {/* ---- Time steps. ------------------------------------------------- */}
      <Box asChild flexGrow="1" width="100%" height="100%">
        <ScrollContainer vertical={false}>
          <Box position="relative" height="100%">
            {iota(alignedNumFrames).map((tickIndex) => {
              const inRange = tickIndex < numFrames;
              const isLarge = tickIndex % 10 === 0;
              const sizeMultiplier = isLarge ? 2 : 1;
              const color = inRange ? "var(--gray-11)" : "var(--gray-8)";

              return (
                <Box
                  key={tickIndex}
                  position="absolute"
                  left={`calc(10px * ${tickIndex})`}
                  top="0"
                  width="10px"
                  height="100%"
                  {...(inRange && {
                    onClick: () => clickOnFrame(tickIndex),
                    className: "rounded hover:bg-gray-600",
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
                    style={{ backgroundColor: color }}
                  />

                  {/* ---- Vertical bar. ----------------------------------- */}
                  <Box
                    position="absolute"
                    top="10%"
                    left="50%"
                    width={`calc(${sizeMultiplier} * 1px)`}
                    height={`calc(${sizeMultiplier} * 20%)`}
                    style={{
                      transform: "translateX(-50%)",
                      backgroundColor: color,
                    }}
                  />

                  {/* ---- Time step index. -------------------------------- */}
                  {isLarge && (
                    <Box
                      position="absolute"
                      left="50%"
                      top="30%"
                      style={{ zIndex: 10, transform: "translateX(-50%)" }}
                    >
                      <TechText size="1" style={{ color }}>
                        {tickIndex}
                      </TechText>
                    </Box>
                  )}

                  {/* ---- Pointer. ---------------------------------------- */}
                  {tickIndex === frameIndex && (
                    <Box
                      position="absolute"
                      width="10px"
                      height="10px"
                      style={{
                        borderTopLeftRadius: "0.25rem",
                        borderTopRightRadius: "0.25rem",
                        backgroundColor: "var(--accent-11)",
                      }}
                    >
                      <Box
                        position="absolute"
                        left="50%"
                        width={`calc(100% / ${Math.SQRT2})`}
                        height={`calc(100% / ${Math.SQRT2})`}
                        style={{
                          backgroundColor: "var(--accent-11)",
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
