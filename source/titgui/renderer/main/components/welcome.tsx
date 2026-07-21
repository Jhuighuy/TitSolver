/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { IconFolderOpen, IconPlus } from "@tabler/icons-react";
import { useAtomValue } from "jotai";

import BlueTitIcon from "~/assets/icon.svg?react";
import { Button } from "~/renderer/common/components/button";
import { chrome } from "~/renderer/common/components/classes";
import { Text } from "~/renderer/common/components/text";
import { cn } from "~/renderer/common/components/utils";
import { formatTimestamp } from "~/renderer/common/utils";
import {
  newCase,
  openCase,
  openRecentCase,
  recentCasesAtom,
} from "~/renderer/main/state/case";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

interface WelcomeProps {
  /** Called after a case was successfully created or opened. */
  onCaseOpened?: () => void;
}

/**
 * The welcome tab: product mark, primary case actions, and the recently
 * opened cases list.
 */
export function Welcome({ onCaseOpened }: Readonly<WelcomeProps>) {
  const recents = useAtomValue(recentCasesAtom);

  const handle = (open: () => Promise<unknown>) => {
    void open().then((state) => {
      if (state !== null) onCaseOpened?.();
    });
  };

  return (
    <div className={cn("size-full overflow-auto", chrome())}>
      <div className="mx-auto flex w-120 max-w-full flex-col gap-10 px-8 py-16">
        {/* ---- Product mark. ----------------------------------------------- */}
        <div className="flex items-center gap-4">
          <BlueTitIcon
            width={48}
            height={48}
            aria-label="BlueTit logo"
            role="img"
          />
          <div className="flex flex-col">
            <Text size="3" weight="bold" color="strong">
              BlueTit Solver
            </Text>
            <Text color="muted">Smoothed particle hydrodynamics studio</Text>
          </div>
        </div>

        {/* ---- Actions. ---------------------------------------------------- */}
        <div className="flex gap-2">
          <Button
            size="2"
            onClick={() => {
              handle(newCase);
            }}
          >
            <IconPlus />
            New Case…
          </Button>
          <Button
            size="2"
            variant="outline"
            onClick={() => {
              handle(openCase);
            }}
          >
            <IconFolderOpen />
            Open Case…
          </Button>
        </div>

        {/* ---- Recents. ---------------------------------------------------- */}
        <div className="flex flex-col gap-2">
          <Text variant="label" color="muted">
            Recent Cases
          </Text>
          {recents.length === 0 ? (
            <Text color="subtle">Cases you open will show up here.</Text>
          ) : (
            <ul className="flex flex-col">
              {recents.map((recent) => (
                <li key={recent.dir}>
                  <button
                    type="button"
                    className="flex w-full cursor-pointer items-baseline gap-3 rounded px-2 py-1.5 text-left transition-colors hover:bg-(--neutral-10)/10"
                    onClick={() => {
                      handle(async () => openRecentCase(recent.dir));
                    }}
                  >
                    <Text color="accent" weight="medium">
                      {recent.name}
                    </Text>
                    <Text color="muted" truncate className="min-w-0 flex-1">
                      {recent.dir}
                    </Text>
                    <Text color="subtle" className="shrink-0">
                      {formatTimestamp(recent.lastOpenedAt)}
                    </Text>
                  </button>
                </li>
              ))}
            </ul>
          )}
        </div>
      </div>
    </div>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
