/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { IconChevronRight } from "@tabler/icons-react";
import {
  createContext,
  useContext,
  useState,
  type ComponentProps,
  type ReactNode,
} from "react";

import { hoverSurface } from "~/renderer-common/components/classes";
import { Text } from "~/renderer-common/components/text";
import { cn } from "~/renderer-common/components/utils";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

const TreeTableLevelContext = createContext(0);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

type TreeTableRootProps = ComponentProps<"div">;

function TreeTableRoot({
  className,
  children,
  ...props
}: Readonly<TreeTableRootProps>) {
  return (
    <TreeTableLevelContext.Provider value={0}>
      <div
        {...props}
        className={cn(
          "grid grid-cols-[max-content_1fr] overflow-hidden",
          className,
        )}
      >
        {children}
      </div>
    </TreeTableLevelContext.Provider>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

interface TreeTableNodeProps {
  label: string;
  icon?: ReactNode;
  value?: ReactNode;
  children?: ReactNode;
}

function TreeTableNode({
  label,
  icon,
  value,
  children,
}: Readonly<TreeTableNodeProps>) {
  const [open, setOpen] = useState(true);
  const level = useContext(TreeTableLevelContext);
  const isExpandable = !!children;
  const chevronLeft = level * INDENT + Math.round(INDENT / 3);

  return (
    <TreeTableLevelContext.Provider value={level + 1}>
      <div
        className={cn(
          "col-span-full grid h-7 grid-cols-subgrid items-center transition-colors",
          hoverSurface(),
        )}
      >
        {/* --- Name. ------------------------------------------------------ */}
        <span
          className="flex items-center gap-1"
          style={{ paddingLeft: chevronLeft }}
        >
          {isExpandable ? (
            <button
              type="button"
              onClick={() => {
                setOpen((prev) => !prev);
              }}
              className="flex shrink-0 cursor-pointer items-center justify-center text-(--fg-5) transition-colors select-none hover:text-(--fg-3)"
              style={{ width: INDENT }}
            >
              <IconChevronRight
                size={Math.round(INDENT * 0.75)}
                className={cn(
                  "shrink-0 transition-transform duration-200",
                  open && "rotate-90",
                )}
              />
            </button>
          ) : (
            <span className="shrink-0" style={{ width: INDENT }} />
          )}
          <span className="flex items-center gap-1.5 pr-2">
            {icon && (
              <span className="shrink-0 text-(--fg-5) [&_svg]:size-[1.5em]">
                {icon}
              </span>
            )}
            <Text>{label}</Text>
          </span>
        </span>

        <span className="flex min-w-0 items-center pr-2">{value}</span>
      </div>

      {/* ---- Children. —-------------------------------------------------- */}
      {isExpandable && (
        <div
          className="col-span-full grid grid-cols-subgrid transition-[grid-template-rows] duration-200 ease-in-out"
          style={{ gridTemplateRows: open ? "1fr" : "0fr" }}
        >
          <div className="col-span-full grid grid-cols-subgrid overflow-hidden">
            <div className="relative col-span-full grid grid-cols-subgrid">
              <div
                className="pointer-events-none absolute inset-y-0 w-px bg-(--bg-6)"
                style={{ left: chevronLeft + Math.round(INDENT / 2) }}
              />
              {children}
            </div>
          </div>
        </div>
      )}
    </TreeTableLevelContext.Provider>
  );
}

const INDENT = 12;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export const TreeTable = Object.assign(TreeTableRoot, {
  Root: TreeTableRoot,
  Node: TreeTableNode,
});

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
