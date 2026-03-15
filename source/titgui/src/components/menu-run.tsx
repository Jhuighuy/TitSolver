/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { Box, Flex, Text } from "@radix-ui/themes";
import { useMemo } from "react";
import {
  Area,
  AreaChart,
  CartesianGrid,
  ResponsiveContainer,
  Tooltip,
  XAxis,
  YAxis,
} from "recharts";
import {
  TbClock as ClockIcon,
  TbCpu as CpuIcon,
  TbCpu2 as MemoryIcon,
  TbRun as RunIcon,
  TbHandStop as StopIcon,
} from "react-icons/tb";

import { surface } from "~/components/classes";
import { type MenuAction, useMenuAction } from "~/components/menu";
import { useSolver } from "~/components/solver";
import { cn } from "~/components/utils";
import { formatDuration, formatMemorySize, formatTimestamp } from "~/utils";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export function RunMenu() {
  const { elapsedMs, isSolverRunning, runSolver, samples, stopSolver } =
    useSolver();

  // ---- Actions. -------------------------------------------------------------

  const runAction = useMemo<MenuAction>(
    () => ({
      name: "Run Solver",
      icon: <RunIcon size={16} />,
      disabled: isSolverRunning,
      onClick: runSolver,
    }),
    [isSolverRunning, runSolver],
  );

  useMenuAction(runAction);

  const stopAction = useMemo<MenuAction>(
    () => ({
      name: "Stop Solver",
      icon: <StopIcon size={16} />,
      disabled: !isSolverRunning,
      onClick: stopSolver,
    }),
    [isSolverRunning, stopSolver],
  );

  useMenuAction(stopAction);

  // ---- Layout. --------------------------------------------------------------

  const latestSample = samples.at(-1);

  return (
    <Flex p="3" gap="3" direction="column" height="100%">
      {/* ---- CPU. -------------------------------------------------------- */}
      <Flex gap="3" direction="row">
        <MetricCard
          icon={<ClockIcon size={16} />}
          title="Elapsed Time"
          value={formatDuration(elapsedMs)}
        />
        <MetricCard
          icon={<CpuIcon size={16} />}
          title="CPU"
          value={
            latestSample === undefined
              ? "0%"
              : `${latestSample.cpuPercent.toFixed(1)}%`
          }
        />
        <MetricCard
          icon={<MemoryIcon size={16} />}
          title="Memory"
          value={
            latestSample === undefined
              ? "0 B"
              : formatMemorySize(latestSample.memoryBytes)
          }
        />
      </Flex>

      <ChartCard
        icon={<CpuIcon size={16} />}
        title="CPU"
        data={samples}
        dataKey="cpuPercent"
        formatValue={(value) => `${value.toFixed(1)}%`}
      />
      <ChartCard
        icon={<MemoryIcon size={16} />}
        title="Memory"
        data={samples}
        dataKey="memoryBytes"
        formatValue={formatMemorySize}
      />
    </Flex>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

type MetricCardProps = {
  icon: React.ReactNode;
  title: string;
  value: string;
};

function MetricCard({ icon, title, value }: Readonly<MetricCardProps>) {
  return (
    <Flex
      flexGrow="1"
      direction="column"
      gap="1"
      p="3"
      width="100%"
      className={cn("rounded-lg", surface())}
    >
      <Text size="1" color="gray">
        <Flex align="center" gap="1">
          {icon}
          {title}
        </Flex>
      </Text>
      <Text size="3" weight="bold">
        {value}
      </Text>
    </Flex>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

type ChartCardProps<K extends string, V> = {
  icon: React.ReactNode;
  title: string;
  data: Array<{
    [key in K]: V;
  }>;
  dataKey: K;
  formatValue: (value: V) => string;
};

function ChartCard<K extends string, V>({
  icon,
  title,
  data,
  dataKey,
  formatValue,
}: Readonly<ChartCardProps<K, V>>) {
  return (
    <Flex
      p="3"
      gap="2"
      direction="column"
      minHeight="220px"
      flexGrow="1"
      className={cn("rounded-lg", surface())}
    >
      <Text size="2" weight="bold">
        <Flex align="center" gap="1">
          {icon}
          {title}
        </Flex>
      </Text>

      <Box flexGrow="1">
        <ResponsiveContainer width="100%" height="100%">
          <AreaChart data={data} className="**:outline-none">
            <CartesianGrid strokeDasharray="3 3" stroke="var(--gray-6)" />

            <XAxis
              dataKey="timestamp"
              tickFormatter={formatTimestamp}
              fontSize={12}
              stroke="var(--gray-11)"
              minTickGap={24}
            />
            <YAxis
              tickFormatter={formatValue}
              fontSize={12}
              stroke="var(--gray-11)"
              width={64}
            />

            <Area
              dataKey={dataKey}
              stroke="var(--accent-9)"
              strokeWidth={1}
              fill="var(--accent-5)"
              fillOpacity={0.5}
              isAnimationActive={false}
            />

            <Tooltip
              cursor={{ stroke: "var(--gray-8)", strokeWidth: 1 }}
              contentStyle={{
                border: "1px solid var(--gray-6)",
                borderRadius: "12px",
                backgroundColor: "var(--gray-2)",
                color: "var(--gray-12)",
              }}
              itemStyle={{ color: "var(--gray-12)" }}
              labelStyle={{ color: "var(--gray-11)" }}
              labelFormatter={(value) => formatTimestamp(Number(value))}
              formatter={(value) => [formatValue(value), title]}
            />
          </AreaChart>
        </ResponsiveContainer>
      </Box>
    </Flex>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
