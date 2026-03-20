/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { Box, ContextMenu, Flex, Text } from "@radix-ui/themes";
import {
  type ColumnDef,
  type Header,
  createColumnHelper,
  getCoreRowModel,
  getSortedRowModel,
  useReactTable,
} from "@tanstack/react-table";
import { useVirtualizer } from "@tanstack/react-virtual";
import {
  type ComponentPropsWithRef,
  type ComponentPropsWithoutRef,
  type CSSProperties,
  type MouseEvent as ReactMouseEvent,
  type ReactNode,
  useMemo,
  useState,
} from "react";
import {
  TbArrowsSort as SortIcon,
  TbChevronDown as SortDescIcon,
  TbChevronUp as SortAscIcon,
  TbColumns3 as CopyColumnIcon,
  TbCopy as CopyValueIcon,
  TbDatabaseExport as CopyVisibleTableIcon,
  TbHash as CopyIdIcon,
  TbTableExport as CopyRowIcon,
  TbTrash as ClearIcon,
} from "react-icons/tb";

import { TechText } from "~/components/basic";
import { useMenuAction, useMenuViewport } from "~/components/menu";
import { useSelection } from "~/components/selection";
import { useStorage } from "~/components/storage";
import type { Field } from "~/visual/fields";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

type ParticleRow = {
  particleIndex: number;
};

type LeafColumnMeta = {
  id: string;
  header: string;
  exportHeader: string;
  getValue: (particleIndex: number) => string;
};

type MenuTarget =
  | { kind: "cell"; particleIndex: number; columnId: string }
  | { kind: "row"; particleIndex: number }
  | { kind: "column"; columnId: string };

type ActiveSelection =
  | { kind: "cell"; particleIndex: number; columnId: string }
  | { kind: "row"; particleIndex: number }
  | { kind: "column"; columnId: string };

type ParticleColumnDef = ColumnDef<ParticleRow, number>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

const columnHelper = createColumnHelper<ParticleRow>();

const idColumnSize = 64;
const scalarColumnSize = 64;
const componentColumnSize = 120;
const rowHeight = 24;
const headerRowHeight = 24;

const headerBackground =
  "color-mix(in srgb, var(--color-panel-solid) 94%, var(--accent-3))";
const stickyColumnBackground = "var(--color-panel-solid)";
const bodyBackground = "var(--color-surface)";
const stickyColumnShadowGradient =
  "linear-gradient(to right, var(--gray-a6), transparent)";

const axes = ["x", "y", "z"] as const;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export function TableMenu() {
  const { frameData } = useStorage();
  const { selectedParticleIndices, clearSelection } = useSelection();
  const scrollElement = useMenuViewport();
  const [activeSelection, setActiveSelection] =
    useState<ActiveSelection | null>(null);

  const filteredParticleIndices = useMemo(
    () =>
      selectedParticleIndices.filter(
        (index) => 0 <= index && index < frameData.count,
      ),
    [frameData.count, selectedParticleIndices],
  );

  const showingSelection = filteredParticleIndices.length > 0;
  const visibleParticleIndices = useMemo(
    () =>
      showingSelection
        ? filteredParticleIndices
        : Array.from({ length: frameData.count }, (_, index) => index),
    [filteredParticleIndices, frameData.count, showingSelection],
  );

  const rows = useMemo<ParticleRow[]>(
    () =>
      visibleParticleIndices.map((particleIndex) => ({
        particleIndex,
      })),
    [visibleParticleIndices],
  );

  const tableDefinition = useMemo(
    () => buildTableDefinition(frameData),
    [frameData],
  );

  // eslint-disable-next-line react-hooks/incompatible-library
  const table = useReactTable({
    data: rows,
    columns: tableDefinition.columns,
    getCoreRowModel: getCoreRowModel(),
    getSortedRowModel: getSortedRowModel(),
    columnResizeMode: "onChange",
    defaultColumn: {
      minSize: 72,
      size: scalarColumnSize,
    },
  });

  const sortedRows = table.getRowModel().rows;
  const visibleSortedParticleIndices = useMemo(
    () => sortedRows.map((row) => row.original.particleIndex),
    [sortedRows],
  );

  const leafColumns = table.getVisibleLeafColumns();
  const idColumn = leafColumns[0];
  const dataColumns = leafColumns.slice(1);
  const idColumnMeta =
    idColumn === undefined
      ? undefined
      : tableDefinition.leafColumnMap.get(idColumn.id);

  const dataColumnLayout = useMemo(() => {
    return dataColumns.reduce<
      { column: (typeof dataColumns)[number]; start: number; size: number }[]
    >((layouts, column) => {
      const previous = layouts.at(-1);
      const start = previous === undefined ? 0 : previous.start + previous.size;
      layouts.push({
        column,
        start,
        size: column.getSize(),
      });
      return layouts;
    }, []);
  }, [dataColumns]);

  const dataColumnStartById = useMemo(
    () =>
      new Map(
        dataColumnLayout.map((layout) => [layout.column.id, layout.start]),
      ),
    [dataColumnLayout],
  );
  const dataColumnSizeById = useMemo(
    () =>
      new Map(
        dataColumnLayout.map((layout) => [layout.column.id, layout.size]),
      ),
    [dataColumnLayout],
  );

  const totalDataWidth = dataColumnLayout.reduce(
    (sum, layout) => sum + layout.size,
    0,
  );
  const totalWidth = (idColumn?.getSize() ?? idColumnSize) + totalDataWidth;

  const headerGroups = table.getHeaderGroups();
  const headerHeight = headerGroups.length * headerRowHeight;

  useMenuAction({
    name: "Clear selection",
    icon: <ClearIcon />,
    disabled: selectedParticleIndices.length === 0,
    onClick: clearSelection,
  });

  const rowVirtualizer = useVirtualizer({
    count: sortedRows.length,
    getScrollElement: () => scrollElement,
    estimateSize: () => rowHeight,
    overscan: 8,
  });

  const columnVirtualizer = useVirtualizer({
    count: dataColumns.length,
    horizontal: true,
    getScrollElement: () => scrollElement,
    estimateSize: (index) => dataColumns[index]?.getSize() ?? scalarColumnSize,
    getItemKey: (index) => dataColumns[index]?.id ?? index,
    overscan: 4,
  });

  return (
    <Box
      width="100%"
      height="100%"
      minWidth="0"
      minHeight="0"
      style={{ fontVariantNumeric: "tabular-nums" }}
    >
      <Box style={{ width: `${totalWidth}px` }}>
        <Box
          position="sticky"
          top="0"
          style={{
            background: headerBackground,
            borderBottom: "1px solid var(--gray-a5)",
            boxShadow:
              "0 1px 0 var(--gray-a5), 0 10px 18px -16px var(--gray-a11)",
            zIndex: 2,
            height: `${headerHeight}px`,
          }}
        >
          {idColumn !== undefined && idColumnMeta !== undefined && (
            <CellContextMenu
              items={
                <CopyMenuItems
                  menuTarget={{ kind: "column", columnId: idColumn.id }}
                  leafColumns={tableDefinition.leafColumns}
                  leafColumnMap={tableDefinition.leafColumnMap}
                  visibleParticleIndices={visibleSortedParticleIndices}
                />
              }
            >
              <StickyCell
                left={0}
                width={idColumn.getSize()}
                height={headerHeight}
                background={getCellBackground(
                  headerBackground,
                  isColumnSelected(activeSelection, idColumn.id),
                )}
                onClick={() =>
                  setActiveSelection({ kind: "column", columnId: idColumn.id })
                }
                onContextMenu={() =>
                  setActiveSelection({ kind: "column", columnId: idColumn.id })
                }
                style={{ zIndex: 3 }}
              >
                <HeaderCell
                  canSort={idColumn.getCanSort()}
                  sortDirection={idColumn.getIsSorted()}
                  canResize={idColumn.getCanResize()}
                  onClick={idColumn.getToggleSortingHandler()}
                  onResize={getResizeHandler(headerGroups[0]?.headers[0])}
                >
                  {idColumnMeta.header}
                </HeaderCell>
              </StickyCell>
            </CellContextMenu>
          )}

          {headerGroups.map((headerGroup, rowIndex) =>
            headerGroup.headers.map((header) => {
              if (header.isPlaceholder) return null;
              const leafIds = getHeaderLeafColumnIds(header);
              if (leafIds.length === 0 || leafIds.includes("id")) return null;

              const left = Math.min(
                ...leafIds.map((id) => dataColumnStartById.get(id) ?? 0),
              );
              const width = leafIds.reduce(
                (sum, id) => sum + (dataColumnSizeById.get(id) ?? 0),
                0,
              );
              const leafColumnId = leafIds.length === 1 ? leafIds[0] : null;
              const rowSpan = Math.max(header.rowSpan, 1);
              const isLeafHeader = leafColumnId !== null;
              const isUngroupedLeafHeader =
                isLeafHeader && header.column.parent === undefined;
              const height = isUngroupedLeafHeader
                ? headerHeight
                : rowSpan * headerRowHeight;
              const top = isUngroupedLeafHeader
                ? 0
                : rowIndex * headerRowHeight;

              const body = (
                <PositionedCell
                  left={(idColumn?.getSize() ?? idColumnSize) + left}
                  top={top}
                  width={width}
                  height={height}
                  background={getCellBackground(
                    headerBackground,
                    isLeafHeader &&
                      leafColumnId !== null &&
                      isColumnSelected(activeSelection, leafColumnId),
                  )}
                  onClick={
                    isLeafHeader && leafColumnId !== null
                      ? () =>
                          setActiveSelection({
                            kind: "column",
                            columnId: leafColumnId,
                          })
                      : undefined
                  }
                  onContextMenu={
                    isLeafHeader && leafColumnId !== null
                      ? () =>
                          setActiveSelection({
                            kind: "column",
                            columnId: leafColumnId,
                          })
                      : undefined
                  }
                >
                  <HeaderCell
                    canSort={header.column.getCanSort()}
                    sortDirection={header.column.getIsSorted()}
                    canResize={header.column.getCanResize()}
                    onClick={
                      isLeafHeader
                        ? header.column.getToggleSortingHandler()
                        : undefined
                    }
                    onResize={getResizeHandler(header)}
                  >
                    {getHeaderLabel(header)}
                  </HeaderCell>
                </PositionedCell>
              );

              if (!isLeafHeader || leafColumnId === null) {
                return <Box key={header.id}>{body}</Box>;
              }

              return (
                <CellContextMenu
                  key={header.id}
                  items={
                    <CopyMenuItems
                      menuTarget={{ kind: "column", columnId: leafColumnId }}
                      leafColumns={tableDefinition.leafColumns}
                      leafColumnMap={tableDefinition.leafColumnMap}
                      visibleParticleIndices={visibleSortedParticleIndices}
                    />
                  }
                >
                  {body}
                </CellContextMenu>
              );
            }),
          )}
        </Box>

        <Box
          position="relative"
          style={{ height: `${rowVirtualizer.getTotalSize()}px` }}
        >
          {rowVirtualizer.getVirtualItems().map((virtualRow) => {
            const row = sortedRows[virtualRow.index];
            if (row === undefined) return null;
            const particleIndex = row.original.particleIndex;

            return (
              <Box
                key={row.id}
                position="absolute"
                width={`${totalWidth}px`}
                style={{
                  transform: `translateY(${virtualRow.start}px)`,
                  height: `${virtualRow.size}px`,
                  borderBottom: "1px solid var(--gray-a4)",
                }}
              >
                {idColumn !== undefined && idColumnMeta !== undefined && (
                  <CellContextMenu
                    items={
                      <CopyMenuItems
                        menuTarget={{ kind: "row", particleIndex }}
                        leafColumns={tableDefinition.leafColumns}
                        leafColumnMap={tableDefinition.leafColumnMap}
                        visibleParticleIndices={visibleSortedParticleIndices}
                      />
                    }
                  >
                    <StickyCell
                      left={0}
                      width={idColumn.getSize()}
                      height={virtualRow.size}
                      background={getCellBackground(
                        stickyColumnBackground,
                        isRowSelected(activeSelection, particleIndex) ||
                          isColumnSelected(activeSelection, idColumn.id),
                      )}
                      onClick={() =>
                        setActiveSelection({ kind: "row", particleIndex })
                      }
                      onContextMenu={() =>
                        setActiveSelection({ kind: "row", particleIndex })
                      }
                      style={{ zIndex: 1 }}
                    >
                      <BodyCell>
                        {idColumnMeta.getValue(particleIndex)}
                      </BodyCell>
                    </StickyCell>
                  </CellContextMenu>
                )}

                {columnVirtualizer.getVirtualItems().map((virtualColumn) => {
                  const column = dataColumns[virtualColumn.index];
                  if (column === undefined) return null;
                  const meta = tableDefinition.leafColumnMap.get(column.id);
                  if (meta === undefined) return null;

                  return (
                    <CellContextMenu
                      key={[row.id, column.id].join(":")}
                      items={
                        <CopyMenuItems
                          menuTarget={{
                            kind: "cell",
                            particleIndex,
                            columnId: column.id,
                          }}
                          leafColumns={tableDefinition.leafColumns}
                          leafColumnMap={tableDefinition.leafColumnMap}
                          visibleParticleIndices={visibleSortedParticleIndices}
                        />
                      }
                    >
                      <PositionedCell
                        left={
                          (idColumn?.getSize() ?? idColumnSize) +
                          virtualColumn.start
                        }
                        top={0}
                        width={virtualColumn.size}
                        height={virtualRow.size}
                        background={getCellBackground(
                          bodyBackground,
                          isRowSelected(activeSelection, particleIndex) ||
                            isColumnSelected(activeSelection, column.id),
                        )}
                        onClick={() =>
                          setActiveSelection({
                            kind: "cell",
                            particleIndex,
                            columnId: column.id,
                          })
                        }
                        onContextMenu={() =>
                          setActiveSelection({
                            kind: "cell",
                            particleIndex,
                            columnId: column.id,
                          })
                        }
                        borderHighlight={isCellSelected(
                          activeSelection,
                          particleIndex,
                          column.id,
                        )}
                      >
                        <BodyCell>{meta.getValue(particleIndex)}</BodyCell>
                      </PositionedCell>
                    </CellContextMenu>
                  );
                })}
              </Box>
            );
          })}
        </Box>
      </Box>
    </Box>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

function buildTableDefinition(frameData: Map<string, Field>) {
  const leafColumns: LeafColumnMeta[] = [
    {
      id: "id",
      header: "#",
      exportHeader: "id",
      getValue: (particleIndex) => String(particleIndex),
    },
  ];

  const columns: ParticleColumnDef[] = [
    columnHelper.accessor((row) => row.particleIndex, {
      id: "id",
      header: "#",
      size: idColumnSize,
      enableSorting: true,
    }),
  ];

  for (const [fieldName, field] of frameData.entries()) {
    if (fieldName === "id") continue;

    switch (field.rank) {
      case 0: {
        const columnId = fieldName;
        leafColumns.push({
          id: columnId,
          header: fieldName,
          exportHeader: fieldName,
          getValue: (particleIndex) => formatFieldValue(field, particleIndex),
        });
        columns.push(
          columnHelper.accessor(
            (row) => Number(field.components(row.particleIndex)[0]),
            {
              id: columnId,
              header: fieldName,
              size: scalarColumnSize,
              enableSorting: true,
            },
          ),
        );
        break;
      }
      case 1: {
        const componentColumns: ParticleColumnDef[] = Array.from(
          { length: field.dim },
          (_, index) => {
            const componentName = axes[index] ?? String(index);
            const columnId = `${fieldName}.${componentName}`;
            leafColumns.push({
              id: columnId,
              header: componentName,
              exportHeader: columnId,
              getValue: (particleIndex) =>
                formatComponentValue(
                  Number(field.components(particleIndex)[index]),
                ),
            });
            return columnHelper.accessor(
              (row) => Number(field.components(row.particleIndex)[index]),
              {
                id: columnId,
                header: componentName,
                size: componentColumnSize,
                enableSorting: true,
              },
            );
          },
        );
        columns.push(
          columnHelper.group({
            id: fieldName,
            header: fieldName,
            columns: componentColumns,
          }),
        );
        break;
      }
      case 2: {
        const componentColumns: ParticleColumnDef[] = Array.from(
          { length: field.width },
          (_, index) => {
            const rowIndex = Math.floor(index / field.dim);
            const columnIndex = index % field.dim;
            const componentName = `${axes[rowIndex] ?? rowIndex}${axes[columnIndex] ?? columnIndex}`;
            const columnId = `${fieldName}.${componentName}`;
            leafColumns.push({
              id: columnId,
              header: componentName,
              exportHeader: columnId,
              getValue: (particleIndex) =>
                formatComponentValue(
                  Number(field.components(particleIndex)[index]),
                ),
            });
            return columnHelper.accessor(
              (row) => Number(field.components(row.particleIndex)[index]),
              {
                id: columnId,
                header: componentName,
                size: componentColumnSize,
                enableSorting: true,
              },
            );
          },
        );
        columns.push(
          columnHelper.group({
            id: fieldName,
            header: fieldName,
            columns: componentColumns,
          }),
        );
        break;
      }
    }
  }

  return {
    columns,
    leafColumns,
    leafColumnMap: new Map(leafColumns.map((column) => [column.id, column])),
  };
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

function CellContextMenu({
  items,
  children,
}: Readonly<{
  items: ReactNode;
  children: ReactNode;
}>) {
  return (
    <ContextMenu.Root modal={false}>
      <ContextMenu.Trigger>{children}</ContextMenu.Trigger>
      <ContextMenu.Content onCloseAutoFocus={(event) => event.preventDefault()}>
        {items}
      </ContextMenu.Content>
    </ContextMenu.Root>
  );
}

function HeaderCell({
  children,
  canSort = false,
  sortDirection = false,
  canResize = false,
  onClick,
  onResize,
}: Readonly<{
  children: ReactNode;
  canSort?: boolean;
  sortDirection?: false | "asc" | "desc";
  canResize?: boolean;
  onClick?: (event: ReactMouseEvent) => void;
  onResize?: (event: ReactMouseEvent) => void;
}>) {
  return (
    <Box
      px="2"
      py="2"
      height="100%"
      style={{ ...contentStyle, position: "relative" }}
    >
      <Text
        size="1"
        weight="bold"
        style={{
          ...contentStyle,
          width: "100%",
          cursor: canSort ? "pointer" : "default",
          userSelect: "none",
        }}
        onClick={onClick}
      >
        <Flex align="center" justify="between" gap="2" width="100%">
          <TechText>{children}</TechText>
          {canSort && <SortIndicator direction={sortDirection} />}
        </Flex>
      </Text>
      {canResize && onResize !== undefined && (
        <Box
          onMouseDown={onResize}
          style={{
            position: "absolute",
            top: 0,
            right: 0,
            width: "8px",
            height: "100%",
            cursor: "col-resize",
            touchAction: "none",
          }}
        />
      )}
    </Box>
  );
}

function SortIndicator({
  direction,
}: Readonly<{ direction: false | "asc" | "desc" }>) {
  switch (direction) {
    case "asc":
      return <SortAscIcon size={14} />;
    case "desc":
      return <SortDescIcon size={14} />;
    default:
      return <SortIcon size={14} opacity={0.55} />;
  }
}

function BodyCell({ children }: Readonly<{ children: string }>) {
  const style: CSSProperties = {
    ...contentStyle,
    overflow: "hidden",
    textOverflow: "ellipsis",
    whiteSpace: "nowrap",
  };
  return (
    <Box px="2" py="6px" height="100%" style={style}>
      <Text size="1" style={style}>
        {children}
      </Text>
    </Box>
  );
}

type BaseCellProps = {
  left: number;
  top?: number;
  width: number;
  height: number;
  background: string;
  onClick?: (event: ReactMouseEvent) => void;
  onContextMenu?: (event: ReactMouseEvent) => void;
  borderHighlight?: boolean;
  children: ReactNode;
} & Omit<
  ComponentPropsWithoutRef<typeof Box>,
  "children" | "left" | "width"
> & {
    ref?: ComponentPropsWithRef<typeof Box>["ref"];
  };

function PositionedCell({
  left,
  top = 0,
  width,
  height,
  background,
  onClick,
  onContextMenu,
  borderHighlight = false,
  children,
  style,
  ref,
  ...props
}: Readonly<BaseCellProps>) {
  return (
    <Box
      {...props}
      ref={ref}
      position="absolute"
      onClick={onClick}
      onContextMenu={onContextMenu}
      style={{
        top: `${top}px`,
        left: `${left}px`,
        width: `${width}px`,
        height: `${height}px`,
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

function StickyCell({
  left,
  width,
  height,
  background,
  onClick,
  onContextMenu,
  children,
  style,
  ref,
  ...props
}: Readonly<BaseCellProps>) {
  return (
    <Box
      {...props}
      ref={ref}
      position="sticky"
      style={{
        left: `${left}px`,
        width: "0",
        height: `${height}px`,
        zIndex: 1,
        overflow: "visible",
        ...style,
      }}
    >
      <Box
        position="absolute"
        top="0"
        left="0"
        width={`${width}px`}
        height={`${height}px`}
        onClick={onClick}
        onContextMenu={onContextMenu}
        style={{
          background,
          borderRight: "1px solid var(--gray-a4)",
          borderBottom: "1px solid var(--gray-a4)",
          boxSizing: "border-box",
        }}
      >
        <Box
          aria-hidden="true"
          style={{
            position: "absolute",
            top: 0,
            right: "-14px",
            width: "14px",
            height: "100%",
            pointerEvents: "none",
            background: stickyColumnShadowGradient,
          }}
        />
        {children}
      </Box>
    </Box>
  );
}

function CopyMenuItems({
  menuTarget,
  leafColumns,
  leafColumnMap,
  visibleParticleIndices,
}: Readonly<{
  menuTarget: MenuTarget;
  leafColumns: LeafColumnMeta[];
  leafColumnMap: Map<string, LeafColumnMeta>;
  visibleParticleIndices: number[];
}>) {
  const copyVisibleTable = () =>
    void copyToClipboard(
      formatVisibleTable(leafColumns, visibleParticleIndices),
    );

  switch (menuTarget.kind) {
    case "cell": {
      const column = leafColumnMap.get(menuTarget.columnId);
      if (column === undefined) return null;
      return (
        <>
          <ContextMenu.Item
            onSelect={() =>
              void copyToClipboard(column.getValue(menuTarget.particleIndex))
            }
          >
            <MenuItemLabel icon={<CopyValueIcon />}>Copy value</MenuItemLabel>
          </ContextMenu.Item>
          <ContextMenu.Item
            onSelect={() =>
              void copyToClipboard(
                formatRow(leafColumns, menuTarget.particleIndex),
              )
            }
          >
            <MenuItemLabel icon={<CopyRowIcon />}>Copy row</MenuItemLabel>
          </ContextMenu.Item>
          <ContextMenu.Item
            onSelect={() =>
              void copyToClipboard(formatColumn(column, visibleParticleIndices))
            }
          >
            <MenuItemLabel icon={<CopyColumnIcon />}>Copy column</MenuItemLabel>
          </ContextMenu.Item>
          <ContextMenu.Item onSelect={copyVisibleTable}>
            <MenuItemLabel icon={<CopyVisibleTableIcon />}>
              Copy visible table
            </MenuItemLabel>
          </ContextMenu.Item>
        </>
      );
    }
    case "row":
      return (
        <>
          <ContextMenu.Item
            onSelect={() =>
              void copyToClipboard(String(menuTarget.particleIndex))
            }
          >
            <MenuItemLabel icon={<CopyIdIcon />}>
              Copy particle id
            </MenuItemLabel>
          </ContextMenu.Item>
          <ContextMenu.Item
            onSelect={() =>
              void copyToClipboard(
                formatRow(leafColumns, menuTarget.particleIndex),
              )
            }
          >
            <MenuItemLabel icon={<CopyRowIcon />}>Copy row</MenuItemLabel>
          </ContextMenu.Item>
          <ContextMenu.Item onSelect={copyVisibleTable}>
            <MenuItemLabel icon={<CopyVisibleTableIcon />}>
              Copy visible table
            </MenuItemLabel>
          </ContextMenu.Item>
        </>
      );
    case "column": {
      const column = leafColumnMap.get(menuTarget.columnId);
      if (column === undefined) return null;
      return (
        <>
          <ContextMenu.Item
            onSelect={() => void copyToClipboard(column.exportHeader)}
          >
            <MenuItemLabel icon={<CopyIdIcon />}>
              Copy column name
            </MenuItemLabel>
          </ContextMenu.Item>
          <ContextMenu.Item
            onSelect={() =>
              void copyToClipboard(formatColumn(column, visibleParticleIndices))
            }
          >
            <MenuItemLabel icon={<CopyColumnIcon />}>Copy column</MenuItemLabel>
          </ContextMenu.Item>
          <ContextMenu.Item onSelect={copyVisibleTable}>
            <MenuItemLabel icon={<CopyVisibleTableIcon />}>
              Copy visible table
            </MenuItemLabel>
          </ContextMenu.Item>
        </>
      );
    }
  }
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

function formatRow(columns: LeafColumnMeta[], particleIndex: number): string {
  return columns
    .map(
      (column) => `${column.exportHeader}\t${column.getValue(particleIndex)}`,
    )
    .join("\n");
}

function formatColumn(
  column: LeafColumnMeta,
  particleIndices: number[],
): string {
  return [
    `id\t${column.exportHeader}`,
    ...particleIndices.map(
      (particleIndex) => `${particleIndex}\t${column.getValue(particleIndex)}`,
    ),
  ].join("\n");
}

function formatVisibleTable(
  columns: LeafColumnMeta[],
  particleIndices: number[],
): string {
  const header = columns.map((column) => column.exportHeader).join("\t");
  const rows = particleIndices.map((particleIndex) =>
    columns.map((column) => column.getValue(particleIndex)).join("\t"),
  );
  return [header, ...rows].join("\n");
}

async function copyToClipboard(text: string): Promise<void> {
  const clipboard = navigator.clipboard;
  if (clipboard === undefined) {
    throw new Error("Clipboard API is unavailable.");
  }
  await clipboard.writeText(text);
}

const contentStyle: CSSProperties = {
  display: "flex",
  alignItems: "center",
  height: "100%",
};

function getCellBackground(baseBackground: string, selected: boolean): string {
  if (!selected) return baseBackground;
  return `color-mix(in srgb, ${baseBackground} 78%, var(--accent-6))`;
}

function isCellSelected(
  selection: ActiveSelection | null,
  particleIndex: number,
  columnId: string,
): boolean {
  return (
    selection?.kind === "cell" &&
    selection.particleIndex === particleIndex &&
    selection.columnId === columnId
  );
}

function isRowSelected(
  selection: ActiveSelection | null,
  particleIndex: number,
): boolean {
  return (
    (selection?.kind === "row" && selection.particleIndex === particleIndex) ||
    (selection?.kind === "cell" && selection.particleIndex === particleIndex)
  );
}

function isColumnSelected(
  selection: ActiveSelection | null,
  columnId: string,
): boolean {
  return (
    (selection?.kind === "column" && selection.columnId === columnId) ||
    (selection?.kind === "cell" && selection.columnId === columnId)
  );
}

function getHeaderLeafColumnIds(
  header: Header<ParticleRow, unknown>,
): string[] {
  if (header.subHeaders.length === 0) return [header.column.id];
  return header.subHeaders.flatMap((child) => getHeaderLeafColumnIds(child));
}

function getHeaderLabel(header: Header<ParticleRow, unknown>): string {
  const value = header.column.columnDef.header;
  return typeof value === "string" ? value : header.column.id;
}

function getResizeHandler(
  header: Header<ParticleRow, unknown> | undefined,
): ((event: ReactMouseEvent) => void) | undefined {
  if (header === undefined || !header.column.getCanResize()) return undefined;
  const resizableHeader = header as Header<ParticleRow, unknown> & {
    getResizeHandler(): (event: ReactMouseEvent) => void;
  };
  return resizableHeader.getResizeHandler();
}

function formatFieldValue(field: Field, index: number): string {
  const components = Array.from(field.components(index), (value) =>
    formatComponentValue(Number(value)),
  );

  switch (field.rank) {
    case 0:
      return components[0] ?? "";
    case 1:
      return `[${components.join(", ")}]`;
    case 2: {
      const rows = [];
      for (let row = 0; row < field.dim; row++) {
        const offset = row * field.dim;
        rows.push(
          `[${components.slice(offset, offset + field.dim).join(", ")}]`,
        );
      }
      return `[${rows.join(", ")}]`;
    }
  }
}

function formatComponentValue(value: number): string {
  if (!Number.isFinite(value)) return String(value);

  const absValue = Math.abs(value);
  if ((absValue !== 0 && absValue < 1e-3) || absValue >= 1e4) {
    return value.toExponential(3);
  }

  return value.toFixed(4).replace(/\.?0+$/, "");
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
