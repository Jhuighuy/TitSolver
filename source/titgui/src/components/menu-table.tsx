/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { Box, DropdownMenu, Flex, Text } from "@radix-ui/themes";
import { useVirtualizer, type VirtualItem } from "@tanstack/react-virtual";
import {
  memo,
  type ComponentPropsWithRef,
  type ComponentPropsWithoutRef,
  type CSSProperties,
  type MouseEvent as ReactMouseEvent,
  type ReactNode,
  useCallback,
  useMemo,
  useState,
} from "react";
import {
  TbArrowsSort as SortIcon,
  TbChevronDown as SortDescIcon,
  TbChevronUp as SortAscIcon,
  TbColumns3 as CopyColumnIcon,
  TbCopy as CopyValueIcon,
  TbHash as CopyIdIcon,
  TbTableExport as CopyRowIcon,
} from "react-icons/tb";

import { TechText } from "~/components/basic";
import { useMenuViewport } from "~/components/menu";
import {
  buildTableModel,
  getSortDirection,
  getSortedParticleIndices,
  tableHeaderHeight,
  tableRowHeight,
  type SortDirection,
  type SortState,
  type TableColumn,
  type TableModel,
  toggleSortState,
} from "~/components/menu-table-model";
import { useStorage } from "~/components/storage";
import { copyToClipboard } from "~/utils";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

type CellTarget = {
  kind: "cell";
  particleIndex: number;
  columnId: string;
};

type ColumnTarget = {
  kind: "column";
  columnId: string;
};

type MenuTarget = CellTarget | ColumnTarget;
type ActiveSelection = CellTarget | ColumnTarget | null;

type MenuPosition = {
  x: number;
  y: number;
};

type MenuMouseEvent = ReactMouseEvent<Element>;
type BoxProps = ComponentPropsWithoutRef<typeof Box>;
type BoxRef = ComponentPropsWithRef<typeof Box>["ref"];

type GridCellProps = Omit<
  BoxProps,
  "as" | "children" | "left" | "top" | "width" | "height"
> & {
  left: number;
  top?: number;
  width: number;
  height: number;
  background: string;
  borderHighlight?: boolean;
  children: ReactNode;
  ref?: BoxRef;
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

const headerBackground =
  "color-mix(in srgb, var(--color-panel-solid) 94%, var(--accent-3))";
const stickyColumnBackground = "var(--color-panel-solid)";
const bodyBackground = "var(--color-surface)";

const alignedCellStyle: CSSProperties = {
  display: "flex",
  alignItems: "center",
  height: "100%",
  overflow: "hidden",
};

const textCellStyle: CSSProperties = {
  ...alignedCellStyle,
  textOverflow: "ellipsis",
  whiteSpace: "nowrap",
};

function px(value: number): string {
  return `${value}px`;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export function TableMenu() {
  const { frameData } = useStorage();
  const scrollElement = useMenuViewport();
  const model = useMemo(() => buildTableModel(frameData), [frameData]);

  const {
    activeSelection,
    sortState,
    menuOpen,
    menuPosition,
    contextTarget,
    activateColumn,
    activateIdColumn,
    activateCell,
    openColumnMenu,
    openCellMenu,
    setMenuOpen,
  } = useTableMenuState();

  const sortedParticleIndices = useMemo(
    () =>
      getSortedParticleIndices(frameData.count, model.leafColumnMap, sortState),
    [frameData.count, model.leafColumnMap, sortState],
  );

  // eslint-disable-next-line react-hooks/incompatible-library
  const rowVirtualizer = useVirtualizer({
    count: sortedParticleIndices.length,
    getScrollElement: () => scrollElement,
    estimateSize: () => tableRowHeight,
    overscan: 10,
  });

  return (
    <>
      <Box
        width="100%"
        height="100%"
        minWidth="0"
        minHeight="0"
        style={{ fontVariantNumeric: "tabular-nums" }}
      >
        <Box width={px(model.totalWidth)}>
          <TableHeader
            model={model}
            activeSelection={activeSelection}
            sortState={sortState}
            onIdColumnClick={activateIdColumn}
            onColumnClick={activateColumn}
            onColumnContextMenu={openColumnMenu}
          />

          <Box position="relative" height={px(rowVirtualizer.getTotalSize())}>
            {rowVirtualizer.getVirtualItems().map((virtualRow) => {
              const particleIndex = sortedParticleIndices[virtualRow.index];
              if (particleIndex === undefined) return null;

              return (
                <TableRow
                  key={particleIndex}
                  particleIndex={particleIndex}
                  virtualRow={virtualRow}
                  model={model}
                  activeSelection={activeSelection}
                  onCellClick={activateCell}
                  onCellContextMenu={openCellMenu}
                />
              );
            })}
          </Box>
        </Box>
      </Box>

      <TableContextMenu
        open={menuOpen}
        position={menuPosition}
        target={contextTarget}
        leafColumns={model.leafColumns}
        leafColumnMap={model.leafColumnMap}
        rowOrder={sortedParticleIndices}
        onOpenChange={setMenuOpen}
      />
    </>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

function useTableMenuState() {
  const [activeSelection, setActiveSelection] = useState<ActiveSelection>(null);
  const [sortState, setSortState] = useState<SortState>(null);
  const [menuOpen, setMenuOpen] = useState(false);
  const [menuPosition, setMenuPosition] = useState<MenuPosition>({
    x: 0,
    y: 0,
  });
  const [contextTarget, setContextTarget] = useState<MenuTarget | null>(null);

  const activateColumn = useCallback((columnId: string) => {
    setActiveSelection({ kind: "column", columnId });
    setSortState((previous) => toggleSortState(previous, columnId));
  }, []);

  const activateIdColumn = useCallback(() => {
    setActiveSelection(null);
    setSortState((previous) => toggleSortState(previous, "id"));
  }, []);

  const activateCell = useCallback(
    (particleIndex: number, columnId: string) => {
      setActiveSelection({ kind: "cell", particleIndex, columnId });
    },
    [],
  );

  const openMenu = useCallback((event: MenuMouseEvent, target: MenuTarget) => {
    event.preventDefault();
    setActiveSelection(target);
    setContextTarget(target);
    setMenuPosition({ x: event.clientX, y: event.clientY });
    setMenuOpen(true);
  }, []);

  const openColumnMenu = useCallback(
    (event: MenuMouseEvent, columnId: string) => {
      openMenu(event, { kind: "column", columnId });
    },
    [openMenu],
  );

  const openCellMenu = useCallback(
    (event: MenuMouseEvent, particleIndex: number, columnId: string) => {
      openMenu(event, { kind: "cell", particleIndex, columnId });
    },
    [openMenu],
  );

  return {
    activeSelection,
    sortState,
    menuOpen,
    menuPosition,
    contextTarget,
    activateColumn,
    activateIdColumn,
    activateCell,
    openColumnMenu,
    openCellMenu,
    setMenuOpen,
  };
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

const TableHeader = memo(function TableHeader({
  model,
  activeSelection,
  sortState,
  onIdColumnClick,
  onColumnClick,
  onColumnContextMenu,
}: Readonly<{
  model: TableModel;
  activeSelection: ActiveSelection;
  sortState: SortState;
  onIdColumnClick: () => void;
  onColumnClick: (columnId: string) => void;
  onColumnContextMenu: (event: MenuMouseEvent, columnId: string) => void;
}>) {
  return (
    <Box
      position="sticky"
      top="0"
      height={px(tableHeaderHeight)}
      style={{
        background: headerBackground,
        borderBottom: "1px solid var(--gray-a5)",
        zIndex: 2,
      }}
    >
      <StickyGridCell
        left={0}
        width={model.idColumn.width}
        height={tableHeaderHeight}
        background={headerBackground}
        onClick={onIdColumnClick}
        style={{ zIndex: 3 }}
      >
        <HeaderContent
          label={model.idColumn.header}
          sortDirection={getSortDirection(sortState, model.idColumn.id)}
        />
      </StickyGridCell>

      {model.dataColumns.map((column) => (
        <PositionedGridCell
          key={column.id}
          left={model.idColumn.width + column.left}
          width={column.width}
          height={tableHeaderHeight}
          background={getCellBackground(
            headerBackground,
            isColumnSelected(activeSelection, column.id),
          )}
          onClick={() => onColumnClick(column.id)}
          onContextMenu={(event: MenuMouseEvent) =>
            onColumnContextMenu(event, column.id)
          }
        >
          <HeaderContent
            label={column.header}
            sortDirection={getSortDirection(sortState, column.id)}
          />
        </PositionedGridCell>
      ))}
    </Box>
  );
});

const TableRow = memo(function TableRow({
  particleIndex,
  virtualRow,
  model,
  activeSelection,
  onCellClick,
  onCellContextMenu,
}: Readonly<{
  particleIndex: number;
  virtualRow: VirtualItem;
  model: TableModel;
  activeSelection: ActiveSelection;
  onCellClick: (particleIndex: number, columnId: string) => void;
  onCellContextMenu: (
    event: MenuMouseEvent,
    particleIndex: number,
    columnId: string,
  ) => void;
}>) {
  return (
    <Box
      position="absolute"
      width={px(model.totalWidth)}
      height={px(virtualRow.size)}
      style={{
        transform: `translateY(${virtualRow.start}px)`,
        borderBottom: "1px solid var(--gray-a4)",
      }}
    >
      <StickyGridCell
        left={0}
        width={model.idColumn.width}
        height={virtualRow.size}
        background={stickyColumnBackground}
        onClick={() => onCellClick(particleIndex, model.idColumn.id)}
        onContextMenu={(event: MenuMouseEvent) =>
          onCellContextMenu(event, particleIndex, model.idColumn.id)
        }
        style={{ zIndex: 1 }}
      >
        <BodyContent>
          {model.idColumn.getDisplayValue(particleIndex)}
        </BodyContent>
      </StickyGridCell>

      {model.dataColumns.map((column) => (
        <TableBodyCell
          key={`${particleIndex}:${column.id}`}
          particleIndex={particleIndex}
          column={column}
          rowHeight={virtualRow.size}
          stickyOffset={model.idColumn.width}
          activeSelection={activeSelection}
          onCellClick={onCellClick}
          onCellContextMenu={onCellContextMenu}
        />
      ))}
    </Box>
  );
});

const TableBodyCell = memo(function TableBodyCell({
  particleIndex,
  column,
  rowHeight,
  stickyOffset,
  activeSelection,
  onCellClick,
  onCellContextMenu,
}: Readonly<{
  particleIndex: number;
  column: TableColumn;
  rowHeight: number;
  stickyOffset: number;
  activeSelection: ActiveSelection;
  onCellClick: (particleIndex: number, columnId: string) => void;
  onCellContextMenu: (
    event: MenuMouseEvent,
    particleIndex: number,
    columnId: string,
  ) => void;
}>) {
  const columnSelected = isColumnSelected(activeSelection, column.id);
  const cellSelected = isCellSelected(
    activeSelection,
    particleIndex,
    column.id,
  );

  return (
    <PositionedGridCell
      left={stickyOffset + column.left}
      width={column.width}
      height={rowHeight}
      background={getCellBackground(bodyBackground, columnSelected)}
      borderHighlight={cellSelected}
      onClick={() => onCellClick(particleIndex, column.id)}
      onContextMenu={(event: MenuMouseEvent) =>
        onCellContextMenu(event, particleIndex, column.id)
      }
    >
      <BodyContent>{column.getDisplayValue(particleIndex)}</BodyContent>
    </PositionedGridCell>
  );
});

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

function TableContextMenu({
  open,
  position,
  target,
  leafColumns,
  leafColumnMap,
  rowOrder,
  onOpenChange,
}: Readonly<{
  open: boolean;
  position: MenuPosition;
  target: MenuTarget | null;
  leafColumns: TableColumn[];
  leafColumnMap: Map<string, TableColumn>;
  rowOrder: number[];
  onOpenChange: (open: boolean) => void;
}>) {
  return (
    <DropdownMenu.Root open={open} onOpenChange={onOpenChange} modal={false}>
      <DropdownMenu.Trigger>
        <Box
          position="fixed"
          left={px(position.x)}
          top={px(position.y)}
          width="1px"
          height="1px"
          style={{ pointerEvents: "none", opacity: 0 }}
        />
      </DropdownMenu.Trigger>
      <DropdownMenu.Content
        onCloseAutoFocus={(event) => event.preventDefault()}
      >
        {target !== null && (
          <CopyMenuItems
            menuTarget={target}
            leafColumns={leafColumns}
            leafColumnMap={leafColumnMap}
            rowOrder={rowOrder}
          />
        )}
      </DropdownMenu.Content>
    </DropdownMenu.Root>
  );
}

function CopyMenuItems({
  menuTarget,
  leafColumns,
  leafColumnMap,
  rowOrder,
}: Readonly<{
  menuTarget: MenuTarget;
  leafColumns: TableColumn[];
  leafColumnMap: Map<string, TableColumn>;
  rowOrder: number[];
}>) {
  switch (menuTarget.kind) {
    case "cell": {
      const column = leafColumnMap.get(menuTarget.columnId);
      if (column === undefined) return null;

      return (
        <>
          <DropdownMenu.Item
            onSelect={() =>
              void copyToClipboard(
                column.getDisplayValue(menuTarget.particleIndex),
              )
            }
          >
            <MenuItemLabel icon={<CopyValueIcon />}>Copy value</MenuItemLabel>
          </DropdownMenu.Item>
          <DropdownMenu.Item
            onSelect={() =>
              void copyToClipboard(
                formatRow(leafColumns, menuTarget.particleIndex),
              )
            }
          >
            <MenuItemLabel icon={<CopyRowIcon />}>Copy row</MenuItemLabel>
          </DropdownMenu.Item>
        </>
      );
    }
    case "column": {
      const column = leafColumnMap.get(menuTarget.columnId);
      if (column === undefined) return null;

      return (
        <>
          <DropdownMenu.Item
            onSelect={() => void copyToClipboard(column.exportHeader)}
          >
            <MenuItemLabel icon={<CopyIdIcon />}>
              Copy column name
            </MenuItemLabel>
          </DropdownMenu.Item>
          <DropdownMenu.Item
            onSelect={() =>
              void copyToClipboard(formatColumn(column, rowOrder))
            }
          >
            <MenuItemLabel icon={<CopyColumnIcon />}>Copy column</MenuItemLabel>
          </DropdownMenu.Item>
        </>
      );
    }
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

function HeaderContent({
  label,
  sortDirection,
}: Readonly<{
  label: string;
  sortDirection: false | SortDirection;
}>) {
  return (
    <Box px="2" py="2" height="100%" style={alignedCellStyle}>
      <Text
        size="1"
        weight="bold"
        style={{
          ...alignedCellStyle,
          width: "100%",
          userSelect: "none",
          cursor: "pointer",
        }}
      >
        <Flex align="center" justify="between" gap="2" width="100%">
          <TechText>{label}</TechText>
          <SortIndicator direction={sortDirection} />
        </Flex>
      </Text>
    </Box>
  );
}

function BodyContent({ children }: Readonly<{ children: string }>) {
  return (
    <Box px="2" py="6px" height="100%" style={textCellStyle}>
      <Text size="1" style={textCellStyle}>
        {children}
      </Text>
    </Box>
  );
}

function SortIndicator({
  direction,
}: Readonly<{ direction: false | SortDirection }>) {
  switch (direction) {
    case "asc":
      return <SortAscIcon size={14} />;
    case "desc":
      return <SortDescIcon size={14} />;
    default:
      return <SortIcon size={14} opacity={0.55} />;
  }
}

function PositionedGridCell({
  left,
  top = 0,
  width,
  height,
  background,
  borderHighlight = false,
  children,
  style,
  ref,
  ...props
}: Readonly<GridCellProps>) {
  return (
    <Box
      {...props}
      ref={ref}
      position="absolute"
      left={px(left)}
      top={px(top)}
      width={px(width)}
      height={px(height)}
      style={{
        background,
        borderRight: "1px solid var(--gray-a4)",
        borderBottom: "1px solid var(--gray-a4)",
        boxSizing: "border-box",
        boxShadow: borderHighlight
          ? "inset 0 0 0 2px var(--accent-8)"
          : undefined,
        zIndex: borderHighlight ? 1 : undefined,
        ...style,
      }}
    >
      {children}
    </Box>
  );
}

function StickyGridCell({
  left,
  top: _top,
  width,
  height,
  background,
  borderHighlight = false,
  children,
  style,
  ref,
  ...props
}: Readonly<GridCellProps>) {
  void _top;

  return (
    <Box
      {...props}
      ref={ref}
      position="sticky"
      left={px(left)}
      height={px(height)}
      style={{
        width: "0",
        overflow: "visible",
        ...style,
      }}
    >
      <Box
        position="absolute"
        top="0"
        left="0"
        width={px(width)}
        height={px(height)}
        style={{
          background,
          borderRight: "1px solid var(--gray-a4)",
          borderBottom: "1px solid var(--gray-a4)",
          boxSizing: "border-box",
          boxShadow: borderHighlight
            ? "inset 0 0 0 2px var(--accent-8)"
            : undefined,
        }}
      >
        {children}
      </Box>
    </Box>
  );
}

function MenuItemLabel({
  icon,
  children,
}: Readonly<{
  icon: ReactNode;
  children: ReactNode;
}>) {
  return (
    <Flex align="center" gap="2">
      {icon}
      <span>{children}</span>
    </Flex>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

function formatRow(columns: TableColumn[], particleIndex: number): string {
  return columns
    .map(
      (column) =>
        `${column.exportHeader}\t${column.getDisplayValue(particleIndex)}`,
    )
    .join("\n");
}

function formatColumn(column: TableColumn, particleIndices: number[]): string {
  return [
    `id\t${column.exportHeader}`,
    ...particleIndices.map(
      (particleIndex) =>
        `${particleIndex}\t${column.getDisplayValue(particleIndex)}`,
    ),
  ].join("\n");
}

function getCellBackground(baseBackground: string, selected: boolean): string {
  if (!selected) return baseBackground;
  return `color-mix(in srgb, ${baseBackground} 78%, var(--accent-6))`;
}

function isCellSelected(
  selection: ActiveSelection,
  particleIndex: number,
  columnId: string,
): boolean {
  return (
    selection?.kind === "cell" &&
    selection.particleIndex === particleIndex &&
    selection.columnId === columnId
  );
}

function isColumnSelected(
  selection: ActiveSelection,
  columnId: string,
): boolean {
  return (
    (selection?.kind === "column" && selection.columnId === columnId) ||
    (selection?.kind === "cell" && selection.columnId === columnId)
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
