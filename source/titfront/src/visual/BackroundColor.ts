/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export type BackgroundColor = {
  name: string;
  color: number | null;
  isDark?: boolean;
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export const backgroundColors = {
  none: {
    name: "None",
    color: null,
    isDark: true,
  } as BackgroundColor,
  warmGray: {
    name: "Warm Gray",
    color: 0x6d6865,
  } as BackgroundColor,
  coolGray: {
    name: "Cool Gray",
    color: 0x5c6279,
  } as BackgroundColor,
  neutralGray: {
    name: "Neutral Gray",
    color: 0x767676,
  } as BackgroundColor,
  lightGray: {
    name: "Light Gray",
    color: 0x807b76,
  } as BackgroundColor,
  white: {
    name: "White",
    color: 0xffffff,
  } as BackgroundColor,
  black: {
    name: "Black",
    color: 0x000000,
    isDark: true,
  } as BackgroundColor,
} as const;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export type BackgroundColorType = keyof typeof backgroundColors;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
