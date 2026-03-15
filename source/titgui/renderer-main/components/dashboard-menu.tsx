/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import {
  IconCancel,
  IconCpu,
  IconCpu2,
  IconRun,
  IconStopwatch,
} from "@tabler/icons-react";
import { useMemo, type ReactNode } from "react";
import {
  Area,
  AreaChart,
  CartesianGrid,
  ResponsiveContainer,
  Tooltip,
  XAxis,
  YAxis,
} from "recharts";

import { Card } from "~/renderer-common/components/card";
import { Box, Flex } from "~/renderer-common/components/layout";
import {
  useMenuAction,
  type MenuAction,
} from "~/renderer-common/components/menu";
import { Text } from "~/renderer-common/components/text";
import {
  formatDuration,
  formatMemorySize,
  formatTimestamp,
} from "~/renderer-common/utils";
import { useSolver } from "~/renderer-main/hooks/use-solver";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export function DashboardMenu() {
  const { elapsedMs, isSolverRunning, runSolver, samples, stopSolver } =
    useSolver();

  // ---- Actions. -------------------------------------------------------------

  const runAction = useMemo<MenuAction>(
    () =>
      isSolverRunning
        ? {
            name: "Stop Solver",
            icon: <IconCancel />,
            onClick: stopSolver,
          }
        : {
            name: "Run Solver",
            icon: <IconRun />,
            onClick: runSolver,
          },
    [isSolverRunning, runSolver, stopSolver],
  );

  useMenuAction(runAction);

  // ---- Layout. --------------------------------------------------------------

  return (
    <Flex direction="column" gap="3" p="3" height="100%" overflow="hidden">
      <Card>
        <Flex align="center" justify="between" p="1">
          <Text size="2" variant="label">
            <Flex align="center" gap="1">
              <IconStopwatch />
              Elapsed
            </Flex>
          </Text>

          <Text size="2" weight="bold">
            {formatDuration(elapsedMs)}
          </Text>
        </Flex>
      </Card>

      <ChartCard
        icon={<IconCpu />}
        title="CPU"
        data={samples}
        dataKey="cpuPercent"
        formatValue={(value) => `${value.toFixed(1)}%`}
      />

      <ChartCard
        icon={<IconCpu2 />}
        title="Memory"
        data={samples}
        dataKey="memoryBytes"
        formatValue={formatMemorySize}
      />
    </Flex>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

interface ChartCardProps<K extends string, V> {
  icon: ReactNode;
  title: string;
  data: Record<K, V>[];
  dataKey: K;
  formatValue: (value: V) => string;
}

function ChartCard<K extends string, V>({
  icon,
  title,
  data,
  dataKey,
  formatValue,
}: Readonly<ChartCardProps<K, V>>) {
  const lastSample = data.at(-1);
  return (
    <Card>
      <Flex direction="column" gap="2" p="1" height="60">
        <Flex align="center" justify="between">
          <Text size="2" variant="label">
            <Flex align="center" gap="1">
              {icon}
              {title}
            </Flex>
          </Text>

          {lastSample !== undefined && (
            <Text size="2" weight="bold">
              {formatValue(lastSample[dataKey])}
            </Text>
          )}
        </Flex>

        <Box minSize="0" flexGrow="1">
          <ResponsiveContainer width="100%" height="100%">
            <AreaChart data={data} className="**:outline-none">
              <CartesianGrid strokeDasharray="3 3" stroke="var(--chrome-1)" />

              <XAxis
                dataKey="timestamp"
                tickFormatter={formatTimestamp}
                fontSize={12}
                stroke="var(--fg-3)"
                minTickGap={24}
              />
              <YAxis
                tickFormatter={formatValue}
                fontSize={12}
                stroke="var(--fg-3)"
                width={64}
              />

              <Area
                dataKey={dataKey}
                stroke="var(--accent-fg-3)"
                strokeWidth={1}
                fill="var(--accent-bg-2)"
                fillOpacity={0.5}
                isAnimationActive={false}
              />

              <Tooltip
                isAnimationActive={false}
                cursor={{ stroke: "var(--chrome-2)", strokeWidth: 1 }}
                contentStyle={{
                  border: "1px solid var(--chrome-1)",
                  borderRadius: "12px",
                  backgroundColor: "var(--bg-3)",
                  color: "var(--fg-1)",
                }}
                itemStyle={{ color: "var(--fg-1)" }}
                labelStyle={{ color: "var(--fg-3)" }}
                labelFormatter={(value) => formatTimestamp(Number(value))}
                formatter={(value) => [formatValue(value as V)]}
              />
            </AreaChart>
          </ResponsiveContainer>
        </Box>
      </Flex>
    </Card>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
