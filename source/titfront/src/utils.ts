/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { type ClassValue, clsx } from "clsx";
import { twMerge } from "tailwind-merge";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
 * Assert that the condition is true.
 */
export function assert(
  condition: unknown,
  message?: string
): asserts condition {
  if (!condition) throw new Error(message ?? "Assertion failed.");
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
 * Generate an array of numbers from `0` to `n - 1`.
 */
export function iota(n: number): number[] {
  return Array.from({ length: n }, (_, i) => i);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
 * Combine class names.
 */
export function cn(...inputs: ClassValue[]): string {
  return twMerge(clsx(inputs));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
 * Flex direction value.
 */
export type FlexDirection = "row" | "row-reverse" | "column" | "column-reverse";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
 * Side of the screen, where a child component can be anchored.
 */
export type Side = "left" | "right" | "top" | "bottom";

/**
 * Convert side to flex direction.
 */
export function sideToFlexDirection(side: Side): FlexDirection {
  switch (side) {
    case "left":
      return "row";
    case "right":
      return "row-reverse";
    case "top":
      return "column";
    case "bottom":
      return "column-reverse";
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
 * Orientation of the component.
 */
export type Orientation = "horizontal" | "vertical";

/**
 * Return opposite orientation.
 */
export function oppositeOrientation(orientation: Orientation): Orientation {
  switch (orientation) {
    case "horizontal":
      return "vertical";
    case "vertical":
      return "horizontal";
  }
}

/**
 * Convert side to orientation.
 */
export function sideToOrientation(side: Side): Orientation {
  switch (side) {
    case "left":
    case "right":
      return "horizontal";
    case "top":
    case "bottom":
      return "vertical";
  }
}

/*
 * Convert orientation to flex direction.
 */
export function orientationToFlexDirection(
  orientation: Orientation
): FlexDirection {
  switch (orientation) {
    case "horizontal":
      return "row";
    case "vertical":
      return "column";
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
