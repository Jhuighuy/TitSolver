/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { useAtom, useAtomValue, useSetAtom } from "jotai";
import {
  type KeyboardEvent,
  type PointerEvent,
  type ReactNode,
  useState,
} from "react";
import { Box2, Vector2 } from "three";

import { Card } from "~/renderer/common/components/card";
import { surface } from "~/renderer/common/components/classes";
import { Strong, Text } from "~/renderer/common/components/text";
import { cn } from "~/renderer/common/components/utils";
import { Polygon2 } from "~/renderer/common/visual/polygon2";
import type {
  SelectionAction,
  SelectionShape,
} from "~/renderer/common/visual/selection";
import {
  type ToolMode,
  useViewportState,
} from "~/renderer/main/state/viewport";
import { assert } from "~/shared/utils";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

type SelectionModeAction = Exclude<SelectionAction, "clear">;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

interface ViewSelectionProps {
  children: ReactNode;
}

export function ViewSelection({ children }: Readonly<ViewSelectionProps>) {
  const viewport = useViewportState();
  const [toolMode, setToolMode] = useAtom(viewport.toolModeAtom);
  const selectionCount = useAtomValue(viewport.selectionCountAtom);
  const emitSelectionCommand = useSetAtom(viewport.selectionCommandAtom);

  // ---- Action. --------------------------------------------------------------

  const [action, setAction] = useState<SelectionModeAction>("replace");

  function setActionFromEvent(event: KeyboardEvent) {
    if (event.altKey) setAction("subtract");
    else if (event.shiftKey) setAction("add");
    else setAction("replace");
  }

  function handleKeyDown(event: KeyboardEvent) {
    if (event.key === "Escape") {
      setToolMode("normal");
      setAction("replace");
      emitSelectionCommand({ action: "clear" });
    } else {
      setActionFromEvent(event);
    }
  }

  function handleKeyUp(event: KeyboardEvent) {
    setActionFromEvent(event);
  }

  // ---- Layout. --------------------------------------------------------------

  const cursor = (() => {
    if (toolMode === "normal") return "default";
    switch (action) {
      case "replace":
        return "crosshair";
      case "add":
        return "copy";
      case "subtract":
        return "not-allowed";
    }
    assert(false);
  })();

  return (
    <div
      tabIndex={-1}
      onPointerDownCapture={(event) => {
        event.currentTarget.focus();
      }}
      onKeyDown={handleKeyDown}
      onKeyUp={handleKeyUp}
      className={cn(
        "relative mx-1 mb-1 grow overflow-hidden focus:outline-none",
        surface(),
      )}
    >
      {children}
      {(selectionCount > 0 || toolMode !== "normal") && (
        <div className="pointer-events-none absolute right-3 bottom-3">
          <SelectionHint
            toolMode={toolMode}
            selectionCount={selectionCount}
            selectionAction={action}
          />
        </div>
      )}
      {toolMode !== "normal" && (
        <div className="absolute inset-0" style={{ cursor }}>
          <SelectionOverlay
            toolMode={toolMode}
            onSelectionShape={(shape) => {
              emitSelectionCommand({ action, shape });
            }}
          />
        </div>
      )}
    </div>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

interface SelectionOverlayProps {
  toolMode: Exclude<ToolMode, "normal">;
  onSelectionShape: (selectionShape: SelectionShape) => void;
}

function SelectionOverlay({
  toolMode,
  onSelectionShape,
}: Readonly<SelectionOverlayProps>) {
  // ---- Shape. ---------------------------------------------------------------

  const [start, setStart] = useState<Vector2 | null>(null);
  const [shape, setShape] = useState<SelectionShape | null>(null);

  function handlePointerDown(event: PointerEvent<HTMLDivElement>) {
    event.currentTarget.setPointerCapture(event.pointerId);

    const rect = event.currentTarget.getBoundingClientRect();
    const start = new Vector2(
      event.clientX - rect.left,
      event.clientY - rect.top,
    );

    switch (toolMode) {
      case "rect":
        setStart(start);
        setShape(new Box2().setFromPoints([start, start.clone()]));
        break;
      case "lasso":
        setStart(null);
        setShape(new Polygon2([start]));
        break;
      default:
        assert(false);
    }
  }

  function handlePointerMove(event: PointerEvent<HTMLDivElement>) {
    if (shape === null) return;

    const rect = event.currentTarget.getBoundingClientRect();
    const point = new Vector2(
      event.clientX - rect.left,
      event.clientY - rect.top,
    );

    switch (toolMode) {
      case "rect": {
        assert(start !== null);
        assert(shape instanceof Box2);
        setShape(new Box2().setFromPoints([start, point]));
        break;
      }
      case "lasso": {
        assert(shape instanceof Polygon2);
        const lastPoint = shape.points.at(-1);
        if (lastPoint === undefined || lastPoint.distanceTo(point) >= 4) {
          setShape(new Polygon2([...shape.points, point]));
        }
        break;
      }
      default:
        assert(false);
    }
  }

  function handlePointerCancel(event: PointerEvent<HTMLDivElement>) {
    if (event.currentTarget.hasPointerCapture(event.pointerId)) {
      event.currentTarget.releasePointerCapture(event.pointerId);
    }

    setStart(null);
    setShape(null);
  }

  function handlePointerUp(event: PointerEvent<HTMLDivElement>) {
    handlePointerCancel(event);
    if (shape !== null) onSelectionShape(shape);
  }

  // ---- Layout. --------------------------------------------------------------

  const svgProps = {
    fill: "color-mix(in srgb, var(--accent-6) 15%, transparent)",
    stroke: "var(--accent-6)",
    strokeWidth: "1.5",
    strokeDasharray: "6 4",
  };

  return (
    <div
      className="size-full"
      tabIndex={-1}
      onPointerDown={handlePointerDown}
      onPointerMove={handlePointerMove}
      onPointerUp={handlePointerUp}
      onPointerCancel={handlePointerCancel}
      onContextMenu={(event) => {
        event.preventDefault();
      }}
      onWheel={(event) => {
        event.preventDefault();
      }}
    >
      <svg width="100%" height="100%" style={{ pointerEvents: "none" }}>
        {shape instanceof Box2 && (
          <rect
            x={shape.min.x}
            y={shape.min.y}
            width={shape.max.x - shape.min.x}
            height={shape.max.y - shape.min.y}
            {...svgProps}
          />
        )}

        {shape instanceof Polygon2 && (
          <polyline
            points={shape.points.map(({ x, y }) => `${x},${y}`).join(" ")}
            {...svgProps}
            strokeLinejoin="round"
          />
        )}
      </svg>
    </div>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

interface SelectionHintProps {
  toolMode: ToolMode;
  selectionCount: number;
  selectionAction: SelectionModeAction;
}

function SelectionHint({
  toolMode,
  selectionCount,
  selectionAction,
}: Readonly<SelectionHintProps>) {
  const toolLabel = (() => {
    switch (toolMode) {
      case "normal":
        return;
      case "rect":
        return "Box Select";
      case "lasso":
        return "Lasso Select";
    }
    assert(false);
  })();

  const actionLabel = (() => {
    switch (selectionAction) {
      case "replace":
        return;
      case "add":
        return "Add";
      case "subtract":
        return "Subtract";
    }
    assert(false);
  })();

  return (
    <Card className="shadow-md">
      <div className="flex flex-col gap-1">
        {toolLabel !== undefined && (
          <Text>
            {toolLabel}
            {actionLabel !== undefined && (
              <>
                {": "}
                <Strong>{actionLabel}</Strong>
              </>
            )}
            {"."}
          </Text>
        )}
        {selectionCount > 0 && (
          <Text>
            <Strong>{selectionCount}</Strong>{" "}
            {selectionCount === 1 ? "particle" : "particles"} selected.
          </Text>
        )}
        {selectionCount > 0 && (
          <Text>
            Press <Strong>Esc</Strong> to clear selection.
          </Text>
        )}
      </div>
    </Card>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
