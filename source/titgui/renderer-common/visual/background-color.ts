/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export type BackgroundColor = {
  label: string;
  color: number | null;
  appearance: "inherit" | "light" | "dark";
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export const backgroundColors = {
  none: {
    label: "None",
    color: null,
    appearance: "inherit",
  } satisfies BackgroundColor,
  warmGray: {
    label: "Warm Gray",
    color: 0x6d6865,
    appearance: "light",
  } satisfies BackgroundColor,
  coolGray: {
    label: "Cool Gray",
    color: 0x5c6279,
    appearance: "light",
  } satisfies BackgroundColor,
  neutralGray: {
    label: "Neutral Gray",
    color: 0x767676,
    appearance: "light",
  } satisfies BackgroundColor,
  lightGray: {
    label: "Light Gray",
    color: 0x807b76,
    appearance: "light",
  } satisfies BackgroundColor,
  white: {
    label: "White",
    color: 0xffffff,
    appearance: "light",
  } satisfies BackgroundColor,
  black: {
    label: "Black",
    color: 0x000000,
    appearance: "dark",
  } satisfies BackgroundColor,
} as const;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export type BackgroundColorName = keyof typeof backgroundColors;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
