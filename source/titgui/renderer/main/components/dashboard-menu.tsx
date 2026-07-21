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
import { useAtomValue } from "jotai";
import { useMemo, type ReactNode } from "react";

import { TimeSeriesChart } from "~/renderer/common/components/chart";
import {
  type MenuAction,
  useMenuAction,
} from "~/renderer/common/components/menu";
import { StatTile } from "~/renderer/common/components/stat-tile";
import { formatDuration, formatMemorySize } from "~/renderer/common/utils";
import {
  isSolverRunningAtom,
  runSolver,
  solverElapsedMsAtom,
  solverSamplesAtom,
  stopSolver,
} from "~/renderer/main/state/solver";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export function DashboardMenu() {
  const isSolverRunning = useAtomValue(isSolverRunningAtom);
  const elapsedMs = useAtomValue(solverElapsedMsAtom);
  const samples = useAtomValue(solverSamplesAtom);

  // ---- Actions. -------------------------------------------------------------

  const runAction = useMemo<MenuAction>(
    () =>
      isSolverRunning
        ? { name: "Stop Solver", icon: <IconCancel />, onClick: stopSolver }
        : { name: "Run Solver", icon: <IconRun />, onClick: runSolver },
    [isSolverRunning],
  );

  useMenuAction(runAction);

  // ---- Layout. --------------------------------------------------------------

  const cpuData = useMemo(
    () =>
      samples.map(({ timestamp, cpuPercent }) => ({
        timestamp,
        value: cpuPercent,
      })),
    [samples],
  );

  const memoryData = useMemo(
    () =>
      samples.map(({ timestamp, memoryBytes }) => ({
        timestamp,
        value: memoryBytes,
      })),
    [samples],
  );

  const lastSample = samples.at(-1);

  return (
    <div className="flex flex-col gap-3 p-2">
      <StatTile
        icon={<IconStopwatch />}
        label="Elapsed"
        value={formatDuration(elapsedMs)}
      />

      <MonitorSection
        icon={<IconCpu />}
        label="CPU"
        value={lastSample && `${lastSample.cpuPercent.toFixed(1)}%`}
      >
        <TimeSeriesChart
          data={cpuData}
          name="CPU"
          formatValue={(value) => `${Math.round(value)}%`}
        />
      </MonitorSection>

      <MonitorSection
        icon={<IconCpu2 />}
        label="Memory"
        value={lastSample && formatMemorySize(lastSample.memoryBytes)}
      >
        <TimeSeriesChart
          data={memoryData}
          name="Memory"
          formatValue={formatMemorySize}
        />
      </MonitorSection>
    </div>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

interface MonitorSectionProps {
  icon: ReactNode;
  label: string;
  value?: ReactNode;
  children: ReactNode;
}

// A monitor: a stat header with the current value, and a chart below.
function MonitorSection({
  icon,
  label,
  value,
  children,
}: Readonly<MonitorSectionProps>) {
  return (
    <section className="flex flex-col gap-1">
      <StatTile icon={icon} label={label} value={value} />
      {children}
    </section>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
