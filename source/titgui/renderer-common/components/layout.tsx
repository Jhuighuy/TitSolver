/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import type {
  ComponentPropsWithoutRef,
  CSSProperties,
  ElementType,
} from "react";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export interface BoxProps extends ComponentPropsWithoutRef<"div"> {
  as?: ElementType;
  // Padding.
  p?: string;
  px?: string;
  py?: string;
  pt?: string;
  pr?: string;
  pb?: string;
  pl?: string;
  // Margin.
  m?: string;
  mx?: string;
  my?: string;
  mt?: string;
  mr?: string;
  mb?: string;
  ml?: string;
  // Sizing.
  size?: string;
  minSize?: string;
  maxSize?: string;
  width?: string;
  height?: string;
  minWidth?: string;
  minHeight?: string;
  maxWidth?: string;
  maxHeight?: string;
  // Flex child.
  flexGrow?: string;
  flexShrink?: string;
  // Position.
  position?: CSSProperties["position"];
  top?: string;
  right?: string;
  bottom?: string;
  left?: string;
  inset?: string;
  // Overflow.
  overflow?: CSSProperties["overflow"];
  overflowX?: CSSProperties["overflowX"];
  overflowY?: CSSProperties["overflowY"];
  // Display.
  display?: CSSProperties["display"];
}

export function Box({
  as: Component = "div",
  p,
  px,
  py,
  pt,
  pr,
  pb,
  pl,
  m,
  mx,
  my,
  mt,
  mr,
  mb,
  ml,
  size,
  minSize,
  maxSize,
  width,
  height,
  minWidth,
  minHeight,
  maxWidth,
  maxHeight,
  flexGrow,
  flexShrink,
  position,
  top,
  right,
  bottom,
  left,
  inset,
  overflow,
  overflowX,
  overflowY,
  display,
  style,
  ...props
}: Readonly<BoxProps>) {
  return (
    <Component
      {...props}
      style={{
        // Padding.
        ...(p !== undefined && { padding: sp(p) }),
        ...(px !== undefined && { paddingLeft: sp(px), paddingRight: sp(px) }),
        ...(py !== undefined && { paddingTop: sp(py), paddingBottom: sp(py) }),
        ...(pt !== undefined && { paddingTop: sp(pt) }),
        ...(pr !== undefined && { paddingRight: sp(pr) }),
        ...(pb !== undefined && { paddingBottom: sp(pb) }),
        ...(pl !== undefined && { paddingLeft: sp(pl) }),
        // Margin.
        ...(m !== undefined && { margin: sp(m) }),
        ...(mx !== undefined && { marginLeft: sp(mx), marginRight: sp(mx) }),
        ...(my !== undefined && { marginTop: sp(my), marginBottom: sp(my) }),
        ...(mt !== undefined && { marginTop: sp(mt) }),
        ...(mr !== undefined && { marginRight: sp(mr) }),
        ...(mb !== undefined && { marginBottom: sp(mb) }),
        ...(ml !== undefined && { marginLeft: sp(ml) }),
        // Sizing.
        ...(size !== undefined && { width: sp(size), height: sp(size) }),
        ...(minSize !== undefined && {
          minWidth: sp(minSize),
          minHeight: sp(minSize),
        }),
        ...(maxSize !== undefined && {
          maxWidth: sp(maxSize),
          maxHeight: sp(maxSize),
        }),
        ...(width !== undefined && { width: sp(width) }),
        ...(height !== undefined && { height: sp(height) }),
        ...(minWidth !== undefined && { minWidth: sp(minWidth) }),
        ...(minHeight !== undefined && { minHeight: sp(minHeight) }),
        ...(maxWidth !== undefined && { maxWidth: sp(maxWidth) }),
        ...(maxHeight !== undefined && { maxHeight: sp(maxHeight) }),
        // Flex child.
        ...(flexGrow !== undefined && { flexGrow }),
        ...(flexShrink !== undefined && { flexShrink }),
        // Position.
        ...(position !== undefined && { position }),
        ...(inset !== undefined && { inset: sp(inset) }),
        ...(top !== undefined && { top: sp(top) }),
        ...(right !== undefined && { right: sp(right) }),
        ...(bottom !== undefined && { bottom: sp(bottom) }),
        ...(left !== undefined && { left: sp(left) }),
        // Overflow.
        ...(overflow !== undefined && { overflow }),
        ...(overflowX !== undefined && { overflowX }),
        ...(overflowY !== undefined && { overflowY }),
        // Display.
        ...(display !== undefined && { display }),
        // Style overrides.
        ...style,
      }}
    />
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

const directionMap = {
  row: "row",
  column: "column",
  "row-reverse": "row-reverse",
  "column-reverse": "column-reverse",
} as const;

type FlexDirection = keyof typeof directionMap;

const alignMap = {
  start: "flex-start",
  end: "flex-end",
  center: "center",
  between: "space-between",
  around: "space-around",
  evenly: "space-evenly",
} as const;

type FlexAlign = keyof typeof alignMap;

const justifyMap = {
  center: "center",
  start: "flex-start",
  end: "flex-end",
  between: "space-between",
  around: "space-around",
  evenly: "space-evenly",
} as const;

type FlexJustify = keyof typeof justifyMap;

const wrapMap = {
  wrap: "wrap",
  nowrap: "nowrap",
  "wrap-reverse": "wrap-reverse",
} as const;

type FlexWrap = keyof typeof wrapMap;

export interface FlexProps extends BoxProps {
  direction?: FlexDirection;
  align?: FlexAlign;
  justify?: FlexJustify;
  wrap?: FlexWrap;
  gap?: string;
  gapX?: string;
  gapY?: string;
}

export function Flex({
  direction = "row",
  align,
  justify,
  style,
  gap,
  gapX,
  gapY,
  wrap,
  ...props
}: Readonly<FlexProps>) {
  return (
    <Box
      display="flex"
      {...props}
      style={{
        flexDirection: directionMap[direction],
        ...(align !== undefined && { alignItems: alignMap[align] }),
        ...(justify !== undefined && { justifyContent: justifyMap[justify] }),
        ...(wrap !== undefined && { flexWrap: wrapMap[wrap] }),
        ...(gap !== undefined && { gap: sp(gap) }),
        ...(gapX !== undefined && { columnGap: sp(gapX) }),
        ...(gapY !== undefined && { rowGap: sp(gapY) }),
        ...style,
      }}
    />
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

function sp(v: string): string {
  const n = Number(v);
  return Number.isFinite(n) ? `${n * 4}px` : v;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
