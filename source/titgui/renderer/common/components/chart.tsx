/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import {
  Area,
  AreaChart,
  CartesianGrid,
  ResponsiveContainer,
  Tooltip,
  XAxis,
  YAxis,
} from "recharts";

import { Text } from "~/renderer/common/components/text";
import { formatTimestamp } from "~/renderer/common/utils";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export interface TimeSeriesPoint {
  timestamp: number;
  value: number;
}

interface TimeSeriesChartProps {
  /** Data points, ordered by timestamp. */
  data: readonly TimeSeriesPoint[];

  /** Series name, shown in the tooltip. */
  name: string;

  /** Value formatter for the axis and the tooltip. */
  formatValue: (value: number) => string;

  /** Chart height in pixels. */
  height?: number;
}

/**
 * A single-series time chart: a 2px line over a light wash of the same hue,
 * with a recessive grid and a crosshair tooltip. The series is drawn in the
 * dedicated chart color token, which is validated for both themes.
 */
export function TimeSeriesChart({
  data,
  name,
  formatValue,
  height = 192,
}: Readonly<TimeSeriesChartProps>) {
  return (
    <ResponsiveContainer width="100%" height={height}>
      <AreaChart
        data={data}
        margin={{ top: 8, right: 8, bottom: 0, left: 0 }}
        className="**:outline-none"
      >
        <CartesianGrid
          stroke="var(--neutral-3)"
          strokeWidth={1}
          vertical={false}
        />

        <XAxis
          dataKey="timestamp"
          tickFormatter={formatTimestamp}
          fontSize="var(--text-1)"
          stroke="var(--neutral-6)"
          tickLine={false}
          minTickGap={32}
        />

        <YAxis
          dataKey="value"
          tickFormatter={formatValue}
          fontSize="var(--text-1)"
          stroke="var(--neutral-6)"
          tickLine={false}
          axisLine={false}
          width={56}
        />

        <Area
          type="monotone"
          dataKey="value"
          stroke="var(--chart-1)"
          strokeWidth={2}
          strokeLinecap="round"
          strokeLinejoin="round"
          fill="var(--chart-1)"
          fillOpacity={0.1}
          isAnimationActive={false}
        />

        <Tooltip
          isAnimationActive={false}
          cursor={{ stroke: "var(--neutral-5)", strokeWidth: 1 }}
          content={<TimeSeriesTooltip name={name} formatValue={formatValue} />}
        />
      </AreaChart>
    </ResponsiveContainer>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

interface TimeSeriesTooltipProps {
  name: string;
  formatValue: (value: number) => string;
  // Injected by the chart's tooltip layer.
  active?: boolean;
  payload?: readonly { value?: unknown }[];
  label?: unknown;
}

// Tooltip readout: the value leads, the series name follows, keyed by a short
// stroke of the series color.
function TimeSeriesTooltip({
  name,
  formatValue,
  active,
  payload,
  label,
}: Readonly<TimeSeriesTooltipProps>) {
  const value = payload?.[0]?.value;
  if (active !== true || typeof value !== "number") return null;

  return (
    <div className="flex flex-col gap-0.5 rounded border border-(--neutral-4) bg-(--neutral-1) px-2 py-1 shadow-(--shadow-popup)">
      <div className="flex items-center gap-1.5">
        <span className="h-0.5 w-3 rounded-full bg-(--chart-1)" />
        <Text weight="bold" color="strong">
          {formatValue(value)}
        </Text>
        <Text color="muted">{name}</Text>
      </div>
      {typeof label === "number" && (
        <Text color="subtle">{formatTimestamp(label)}</Text>
      )}
    </div>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
