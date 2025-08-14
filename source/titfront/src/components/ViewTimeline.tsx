/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import {
  Box,
  Flex,
  IconButton,
  Separator,
  TextField,
  Tooltip,
} from "@radix-ui/themes";
import {
  type ChangeEvent,
  type ComponentProps,
  useEffect,
  useMemo,
  useState,
} from "react";
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
import {
  TbMultiplier05X as MultiplierIcon0p5x,
  TbMultiplier15X as MultiplierIcon1p5x,
  TbMultiplier1X as MultiplierIcon1x,
  TbMultiplier2X as MultiplierIcon2x,
} from "react-icons/tb";
import ScrollContainer from "react-indiana-drag-scroll";

import { TechnicalText } from "~/components/Basic";
import { useServer } from "~/components/Server";
import { type DataSet, useDataStore } from "~/stores/DataStore";
import { assert, iota } from "~/utils";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

type ViewTimelineProps = {
  boxWidth?: string;
  bigTickFreq?: number;
  bigTickMultiplier?: number;
};

export function ViewTimeline({
  boxWidth = "10px",
  bigTickFreq = 10,
  bigTickMultiplier = 2,
}: Readonly<ViewTimelineProps>) {
  // --- Playback handling. ----------------------------------------------------

  const playbackSpeeds = useMemo(
    () => [
      { multiplier: 0.5, icon: <MultiplierIcon0p5x size={16} /> },
      { multiplier: 1, icon: <MultiplierIcon1x size={16} /> },
      { multiplier: 1.5, icon: <MultiplierIcon1p5x size={16} /> },
      { multiplier: 2, icon: <MultiplierIcon2x size={16} /> },
    ],
    []
  );

  const [playbackSpeedIndex, setPlaybackSpeedIndex] = useState(1);

  function setNextPlaybackSpeed() {
    setPlaybackSpeedIndex((prev) => (prev + 1) % playbackSpeeds.length);
  }

  const [isPlaying, setIsPlaying] = useState(false);
  const [isRepeating, setIsRepeating] = useState(false);
  const {
    numTimeSteps,
    setNumTimeSteps,
    requestedTimeStep,
    requestTimeStep,
    currentTimeStep,
    resolveTimeStep,
    isFrameCached,
  } = useDataStore();

  const alignedNumTimeSteps = 10 ** Math.ceil(Math.log10(numTimeSteps)) + 1;

  function togglePlay() {
    if (isPlaying) {
      setIsPlaying(false);
    } else {
      if (currentTimeStep === numTimeSteps - 1) {
        requestTimeStep(0);
      }
      setIsPlaying(true);
    }
  }

  function setNextTimeStep() {
    assert(currentTimeStep !== null);
    setIsPlaying(false);
    requestTimeStep(
      isRepeating
        ? (currentTimeStep + 1) % numTimeSteps
        : Math.min(numTimeSteps - 1, currentTimeStep + 1)
    );
  }

  function setPreviousTimeStep() {
    assert(currentTimeStep !== null);
    setIsPlaying(false);
    requestTimeStep(
      isRepeating
        ? (currentTimeStep + numTimeSteps - 1) % numTimeSteps
        : Math.max(0, currentTimeStep - 1)
    );
  }

  // If playing, request the next time step once the current one is ready.
  useEffect(() => {
    if (!isPlaying) return;
    if (currentTimeStep === null || requestedTimeStep !== null) return;

    if (!isRepeating && currentTimeStep === numTimeSteps - 1) {
      setIsPlaying(false);
      return;
    }

    const nextTimeStep = isRepeating
      ? (currentTimeStep + 1) % numTimeSteps
      : currentTimeStep + 1;
    if (isFrameCached(nextTimeStep)) {
      const fps = 30 * playbackSpeeds[playbackSpeedIndex].multiplier;
      const delay = 1000 / fps;
      const interval = setInterval(() => requestTimeStep(nextTimeStep), delay);
      return () => clearInterval(interval);
    } else {
      requestTimeStep(nextTimeStep);
    }
  }, [
    isPlaying,
    isRepeating,
    playbackSpeeds,
    playbackSpeedIndex,
    numTimeSteps,
    currentTimeStep,
    requestedTimeStep,
    requestTimeStep,
    isFrameCached,
  ]);

  // --- Timestep input handling. ----------------------------------------------

  const [currentTimeStepString, setCurrentTimeStepString] = useState("");
  const [currentTimeStepStringIsValid, setCurrentTimeStepStringIsValid] =
    useState(true);

  useEffect(() => {
    setCurrentTimeStepString(`${currentTimeStep}`);
  }, [currentTimeStep]);

  // Handle the input and validate it. Do not request the time step yet.
  function handleCurrentTimeStepStringInputChange(
    event: ChangeEvent<HTMLInputElement>
  ) {
    const inputValue = event.target.value;
    setCurrentTimeStepString(inputValue);

    if (/^\d+$/.test(inputValue)) {
      const inputValueInt = Number.parseInt(inputValue, 10);
      if (inputValueInt < numTimeSteps) {
        setCurrentTimeStepStringIsValid(true);
        return;
      }
    }

    setCurrentTimeStepStringIsValid(false);
  }

  // Request the time step when the input loses focus.
  function handleCurrentTimeStepStringInputBlur() {
    if (currentTimeStepStringIsValid) {
      const inputValueInt = Number.parseInt(currentTimeStepString, 10);
      assert(0 <= inputValueInt && inputValueInt < numTimeSteps);
      requestTimeStep(inputValueInt);
    } else {
      setCurrentTimeStepString(`${currentTimeStep}`);
      setCurrentTimeStepStringIsValid(true);
    }
  }

  // ---- Data. ----------------------------------------------------------------

  const runCode = useServer();

  // Fetch the number of time steps from the server.
  useEffect(() => {
    runCode("", (result) => {
      const numTimeSteps = result as number;
      setNumTimeSteps(numTimeSteps);
    });
  }, [runCode, setNumTimeSteps]);

  // If the frame is requested, fetch it from the server.
  /** @todo This is probably not the best place to handle this logic. */
  useEffect(() => {
    if (requestedTimeStep === null) return;
    runCode(`#${requestedTimeStep}`, (result) => {
      const dataset = result as DataSet;
      resolveTimeStep(requestedTimeStep, dataset);
    });
  }, [runCode, requestedTimeStep, resolveTimeStep]);

  // ---- Layout. --------------------------------------------------------------

  return (
    <Flex
      direction="row"
      align="center"
      height="36px"
      px="2"
      py="4px"
      gap="1"
      className="bg-gradient-to-br from-gray-700 to-gray-800 inset-shadow-sm inset-shadow-gray-700"
    >
      {/* ---- Playback controls. ------------------------------------------ */}
      <Flex direction="row" align="center" gap="4" mx="2">
        {/* ---- Repeat. --------------------------------------------------- */}
        <Tooltip content="Toggle repeat">
          <IconButton
            variant="ghost"
            color={isRepeating ? "blue" : "gray"}
            radius="full"
            onClick={() => setIsRepeating(!isRepeating)}
          >
            <RepeatIcon size={16} />
          </IconButton>
        </Tooltip>
        <Separator orientation="vertical" size="1" />

        {/* ---- Back / Previous / (Pause | Play) / Next / End. ------------ */}
        <Tooltip content="Go to start">
          <IconButton
            variant="ghost"
            color="gray"
            radius="full"
            onClick={() => requestTimeStep(0)}
          >
            <BackIcon size={16} />
          </IconButton>
        </Tooltip>
        <Separator orientation="vertical" size="1" />

        <Tooltip content="Go to previous step">
          <IconButton
            variant="ghost"
            color="gray"
            radius="full"
            disabled={currentTimeStep === null}
            onClick={setPreviousTimeStep}
          >
            <StepBackwardIcon size={16} />
          </IconButton>
        </Tooltip>
        <Separator orientation="vertical" size="1" />

        <Tooltip content={isPlaying ? "Pause" : "Play"}>
          <IconButton
            variant="ghost"
            color="gray"
            radius="full"
            disabled={currentTimeStep === null}
            onClick={togglePlay}
          >
            {isPlaying ? <PauseIcon size={16} /> : <PlayIcon size={16} />}
          </IconButton>
        </Tooltip>
        <Separator orientation="vertical" size="1" />

        <Tooltip content="Go to next step">
          <IconButton
            variant="ghost"
            color="gray"
            radius="full"
            disabled={currentTimeStep === null && numTimeSteps === 0}
            onClick={setNextTimeStep}
          >
            <StepForwardIcon size={16} />
          </IconButton>
        </Tooltip>
        <Separator orientation="vertical" size="1" />

        <Tooltip content="Go to end">
          <IconButton
            variant="ghost"
            color="gray"
            radius="full"
            disabled={numTimeSteps === 0}
            onClick={() => requestTimeStep(numTimeSteps - 1)}
          >
            <ForwardIcon size={16} />
          </IconButton>
        </Tooltip>
        <Separator orientation="vertical" size="1" />

        {/* ---- Playback speed. ------------------------------------------- */}
        <Tooltip content="Playback speed">
          <IconButton
            variant="ghost"
            color="gray"
            radius="full"
            onClick={setNextPlaybackSpeed}
          >
            {playbackSpeeds[playbackSpeedIndex].icon}
          </IconButton>
        </Tooltip>
      </Flex>
      <Separator orientation="vertical" size="4" />

      {/* ---- Time step selection. ---------------------------------------- */}
      <Flex direction="row" align="center" gap="2" mx="2">
        <Tooltip content="Refresh">
          <IconButton variant="ghost" color="gray" radius="full">
            <RefreshIcon size={16} />
          </IconButton>
        </Tooltip>
        <TextField.Root
          size="1"
          radius="full"
          placeholder="…"
          value={currentTimeStepString}
          color={currentTimeStepStringIsValid ? undefined : "red"}
          onChange={handleCurrentTimeStepStringInputChange}
          onBlur={handleCurrentTimeStepStringInputBlur}
        />
      </Flex>
      <Separator orientation="vertical" size="4" />

      {/* ---- Time steps. ------------------------------------------------- */}
      <Box asChild flexGrow="1" width="100%" height="100%">
        <ScrollContainer vertical={false}>
          <Box position="relative" height="100%">
            {iota(alignedNumTimeSteps).map((timeStep) => {
              const inRange = timeStep < numTimeSteps;
              const isBigTick = timeStep % bigTickFreq === 0;
              const tickMultiplier = isBigTick ? bigTickMultiplier : 1;
              const color = inRange ? "var(--gray-11)" : "var(--gray-8)";

              return (
                <Box
                  key={timeStep}
                  position="absolute"
                  left={`calc(${boxWidth} * ${timeStep})`}
                  top="0"
                  width={boxWidth}
                  height="100%"
                  {...(inRange && {
                    onClick: () => requestTimeStep(timeStep),
                    className: "rounded hover:bg-gray-600",
                  })}
                >
                  {/* ---- Horizontal bar. --------------------------------- */}
                  <Box
                    position="absolute"
                    left={0 < timeStep ? "0" : "50%"}
                    top="10%"
                    width={
                      0 < timeStep && timeStep < alignedNumTimeSteps - 1
                        ? "100%"
                        : "50%"
                    }
                    height="2px"
                    style={{ backgroundColor: color }}
                  />

                  {/* ---- Vertical bar. ----------------------------------- */}
                  <Box
                    position="absolute"
                    top="10%"
                    left="50%"
                    width={`calc(${tickMultiplier} * 1px)`}
                    height={`calc(${tickMultiplier} * 20%)`}
                    style={{
                      transform: "translateX(-50%)",
                      backgroundColor: color,
                    }}
                  />

                  {/* ---- Time step index. -------------------------------- */}
                  {isBigTick && (
                    <Box
                      position="absolute"
                      left="50%"
                      top="30%"
                      style={{ zIndex: 10, transform: "translateX(-50%)" }}
                    >
                      <TechnicalText size="1" style={{ color }}>
                        {timeStep}
                      </TechnicalText>
                    </Box>
                  )}

                  {/* ---- Pointer. ---------------------------------------- */}
                  {timeStep === currentTimeStep && (
                    <Pointer
                      position="absolute"
                      width={boxWidth}
                      height={boxWidth}
                    />
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

function Pointer({ ...props }: ComponentProps<typeof Box>) {
  return (
    <Box
      {...props}
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
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
