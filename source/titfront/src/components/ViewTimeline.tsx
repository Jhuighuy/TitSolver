/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import {
  Box,
  Flex,
  IconButton,
  Separator,
  TextField,
  Tooltip,
} from "@radix-ui/themes";
import { type ChangeEvent, useEffect, useMemo, useState } from "react";
import {
  FaPlus as PlusIcon,
  FaMinus as MinusIcon,
  FaBackward as BackIcon,
  FaForward as ForwardIcon,
  FaPause as PauseIcon,
  FaPlay as PlayIcon,
  FaStepForward as StepForwardIcon,
  FaStepBackward as StepBackwardIcon,
} from "react-icons/fa";
import { FaRepeat as RepeatIcon } from "react-icons/fa6";
import {
  TbMultiplier05X as MultiplierIcon_0_5x,
  TbMultiplier1X as MultiplierIcon_1_0x,
  TbMultiplier15X as MultiplierIcon_1_5x,
  TbMultiplier2X as MultiplierIcon_2_0x,
} from "react-icons/tb";
import ScrollContainer from "react-indiana-drag-scroll";

import { useServer } from "~/components/Server";
import { type DataSet, useDataStore } from "~/stores/DataStore";
import { assert, cn, iota } from "~/utils";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export interface ViewTimelineProps {
  boxWidthPx?: number;
  tickWidthPx?: number;
  tickHeightPercent?: number;
  bigTickFreq?: number;
  bigTickMultiplier?: number;
}

export function ViewTimeline({
  boxWidthPx = 10,
  tickWidthPx = 1,
  tickHeightPercent = 40,
  bigTickFreq = 10,
  bigTickMultiplier = 2,
}: ViewTimelineProps) {
  // --- Playback handling. ----------------------------------------------------

  const playbackSpeeds = useMemo(
    () => [
      { multiplier: 0.5, icon: <MultiplierIcon_0_5x size={16} /> },
      { multiplier: 1.0, icon: <MultiplierIcon_1_0x size={16} /> },
      { multiplier: 1.5, icon: <MultiplierIcon_1_5x size={16} /> },
      { multiplier: 2.0, icon: <MultiplierIcon_2_0x size={16} /> },
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
  }, [currentTimeStep, setCurrentTimeStepString]);

  // Handle the input and validate it. Do not request the time step yet.
  function handleCurrentTimeStepStringInputChange(
    event: ChangeEvent<HTMLInputElement>
  ) {
    const inputValue = event.target.value;
    setCurrentTimeStepString(inputValue);

    if (/^\d+$/.test(inputValue)) {
      const inputValueInt = Number.parseInt(inputValue);
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
      const inputValueInt = Number.parseInt(currentTimeStepString);
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
      className="bg-gradient-to-bl from-gray-800 to-gray-700"
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
        <Tooltip content="Go to previous step">
          <IconButton
            variant="ghost"
            color="gray"
            radius="full"
            onClick={setPreviousTimeStep}
          >
            <MinusIcon size={16} />
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
        <Tooltip content="Go to next step">
          <IconButton
            variant="ghost"
            color="gray"
            radius="full"
            onClick={setNextTimeStep}
          >
            <PlusIcon size={16} />
          </IconButton>
        </Tooltip>
      </Flex>
      <Separator orientation="vertical" size="4" />

      {/* ---- Ticks. ------------------------------------------------------ */}
      <Box asChild flexGrow="1">
        {/** @todo Eventually, I would like to add wheel scrolling support. */}
        <ScrollContainer vertical={false} className="size-full">
          <Box height="100%" className="relative opacity-75">
            {iota(numTimeSteps).map((timeStep) => {
              const isBigTick =
                timeStep % bigTickFreq === 0 || timeStep === numTimeSteps - 1;
              const thisTickMultiplier = isBigTick ? bigTickMultiplier : 1;
              const thisTickWidthPx = thisTickMultiplier * tickWidthPx;
              const thisTickHeightPercent =
                thisTickMultiplier * tickHeightPercent;

              return (
                <div
                  key={timeStep}
                  onClick={() => requestTimeStep(timeStep)}
                  className={cn(
                    "absolute h-full hover:bg-gray-600",
                    timeStep === currentTimeStep && "bg-indigo-500"
                  )}
                  style={{
                    top: "1px",
                    left: `${boxWidthPx * timeStep}px`,
                    width: `${boxWidthPx}px`,
                  }}
                >
                  <div
                    className="bg-gray-300 absolute top-0"
                    style={{
                      top: "1px",
                      left: `calc(50% - ${thisTickWidthPx / 2}px)`,
                      width: `${thisTickWidthPx}px`,
                      height: `${thisTickHeightPercent}%`,
                    }}
                  />
                  {isBigTick && (
                    <span className="absolute left-[110%] top-[50%] z-10 text-xs font-mono italic text-gray-300 pointer-events-none">
                      {timeStep}
                    </span>
                  )}
                </div>
              );
            })}
          </Box>
        </ScrollContainer>
      </Box>
    </Flex>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
