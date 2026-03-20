/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { formatNumber, iota } from "~/utils";
import type { Field } from "~/visual/fields";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export type SortDirection = "asc" | "desc";

export type SortState = {
  columnId: string;
  direction: SortDirection;
} | null;

export type TableColumn = {
  id: string;
  header: string;
  exportHeader: string;
  left: number;
  width: number;
  getNumericValue: (particleIndex: number) => number;
  getDisplayValue: (particleIndex: number) => string;
};

export type TableModel = {
  idColumn: TableColumn;
  dataColumns: TableColumn[];
  leafColumns: TableColumn[];
  leafColumnMap: Map<string, TableColumn>;
  totalWidth: number;
};

export const tableHeaderHeight = 24;
export const tableRowHeight = 24;

const idColumnWidth = 64;
const scalarColumnWidth = 120;
const componentColumnWidth = 120;

const axes = ["x", "y", "z"] as const;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export function buildTableModel(frameData: Map<string, Field>): TableModel {
  const idColumn = createIdColumn();
  const dataColumns = createDataColumns(frameData);
  const leafColumns = [idColumn, ...dataColumns];

  return {
    idColumn,
    dataColumns,
    leafColumns,
    leafColumnMap: new Map(leafColumns.map((column) => [column.id, column])),
    totalWidth:
      idColumn.width +
      dataColumns.reduce((sum, column) => sum + column.width, 0),
  };
}

export function getSortedParticleIndices(
  count: number,
  leafColumnMap: Map<string, TableColumn>,
  sortState: SortState,
): number[] {
  const indices = iota(count);
  if (sortState === null) return indices;

  const column = leafColumnMap.get(sortState.columnId);
  if (column === undefined) return indices;

  indices.sort((leftIndex, rightIndex) => {
    const leftValue = column.getNumericValue(leftIndex);
    const rightValue = column.getNumericValue(rightIndex);

    if (Number.isNaN(leftValue) && Number.isNaN(rightValue)) {
      return leftIndex - rightIndex;
    }
    if (Number.isNaN(leftValue)) return 1;
    if (Number.isNaN(rightValue)) return -1;
    if (leftValue < rightValue) {
      return sortState.direction === "asc" ? -1 : 1;
    }
    if (leftValue > rightValue) {
      return sortState.direction === "asc" ? 1 : -1;
    }
    return leftIndex - rightIndex;
  });

  return indices;
}

export function toggleSortState(
  previous: SortState,
  columnId: string,
): SortState {
  if (previous === null || previous.columnId !== columnId) {
    return { columnId, direction: "asc" };
  }
  if (previous.direction === "asc") {
    return { columnId, direction: "desc" };
  }
  return null;
}

export function getSortDirection(
  sortState: SortState,
  columnId: string,
): false | SortDirection {
  return sortState?.columnId === columnId ? sortState.direction : false;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

function createIdColumn(): TableColumn {
  return {
    id: "id",
    header: "#",
    exportHeader: "id",
    left: 0,
    width: idColumnWidth,
    getNumericValue: (particleIndex) => particleIndex,
    getDisplayValue: (particleIndex) => String(particleIndex),
  };
}

function createDataColumns(frameData: Map<string, Field>): TableColumn[] {
  let left = 0;
  const columns: TableColumn[] = [];

  for (const [fieldName, field] of frameData.entries()) {
    if (fieldName === "id") continue;

    switch (field.rank) {
      case 0:
        columns.push(createFieldColumn(fieldName, field, 0, fieldName, left));
        left += scalarColumnWidth;
        break;
      case 1:
        for (
          let componentIndex = 0;
          componentIndex < field.dim;
          componentIndex++
        ) {
          const axis = axes[componentIndex] ?? String(componentIndex);
          const columnId = `${fieldName}.${axis}`;
          columns.push(
            createFieldColumn(columnId, field, componentIndex, columnId, left),
          );
          left += componentColumnWidth;
        }
        break;
      case 2:
        for (
          let componentIndex = 0;
          componentIndex < field.width;
          componentIndex++
        ) {
          const row = Math.floor(componentIndex / field.dim);
          const col = componentIndex % field.dim;
          const axis = `${axes[row] ?? row}${axes[col] ?? col}`;
          const columnId = `${fieldName}.${axis}`;
          columns.push(
            createFieldColumn(columnId, field, componentIndex, columnId, left),
          );
          left += componentColumnWidth;
        }
        break;
    }
  }

  return columns;
}

function createFieldColumn(
  id: string,
  field: Field,
  componentIndex: number,
  header: string,
  left: number,
): TableColumn {
  const data = field.data;
  const width = field.width;

  return {
    id,
    header,
    exportHeader: id,
    left,
    width: field.rank === 0 ? scalarColumnWidth : componentColumnWidth,
    getNumericValue: (particleIndex) =>
      Number(data[particleIndex * width + componentIndex]),
    getDisplayValue: (particleIndex) =>
      formatNumber(Number(data[particleIndex * width + componentIndex])),
  };
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
