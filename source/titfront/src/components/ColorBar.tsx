/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { Fragment, type ReactNode } from "react";

import { assert, cn } from "~/utils";
import { type ColorMapType, colorMaps } from "~/visual/ColorMap";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export interface ColorBarProps {
  colorMap: ColorMapType;
  widthPx: number;
  heightPx: number;
  children?: ReactNode;
}

export function ColorBar({
  colorMap,
  widthPx: width,
  heightPx: height,
  children,
}: ColorBarProps) {
  const { rgbPoints } = colorMaps[colorMap];
  const gradientStops = rgbPoints
    .map(
      ([v, r, g, b]) =>
        `rgb(
          ${Math.round(r * 255)},
          ${Math.round(g * 255)},
          ${Math.round(b * 255)}
        ) ${v * 100.0}%`
    )
    .join(", ");

  const vertical = width < height;
  const gradient = vertical
    ? `linear-gradient(to top, ${gradientStops})`
    : `linear-gradient(to right, ${gradientStops})`;

  return <div style={{ width, height, background: gradient }}>{children}</div>;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export interface ColorBarWithTicks {
  colorMap: ColorMapType;
  min: number;
  max: number;
  ticks?: number;
  dark?: boolean;
  widthPx: number;
  heightPx: number;
}

export function ColorBarWithTicks({
  colorMap,
  min,
  max,
  ticks = 10,
  dark = false,
  widthPx,
  heightPx,
}: ColorBarWithTicks) {
  assert(heightPx > widthPx, "Height must be greater than width for ticks.");

  /** @todo Refactor with iota. */
  const tickValues = Array.from(
    { length: ticks },
    (_, i) => min + ((max - min) * i) / (ticks - 1)
  );

  return (
    <ColorBar colorMap={colorMap} widthPx={widthPx} heightPx={heightPx}>
      <div className="relative size-full opacity-75">
        {tickValues.map((value) => {
          const t = (value - min) / (max - min);

          const tick = (
            <div
              className={cn("absolute", dark ? "bg-gray-300" : "bg-black")}
              style={{
                top: `${(1 - t) * 100}%`,
                left: `${widthPx / 2}px`,
                height: "1px",
                width: `${widthPx}px`,
              }}
            />
          );

          const label = (
            <span
              className={cn(
                "absolute text-xs font-mono italic",
                dark ? "text-gray-300" : "text-black"
              )}
              style={{
                top: `${(1 - t) * 100}%`,
                left: `${1.75 * widthPx}px`,
                transform: "translateY(-50%)",
              }}
            >
              {value.toFixed(2)}
            </span>
          );

          return (
            <Fragment key={value}>
              {tick}
              {label}
            </Fragment>
          );
        })}
      </div>
    </ColorBar>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
