/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import type { ReactNode } from "react";

import BlueTitIcon from "~/assets/icon.svg?react";
import { chrome } from "~/renderer/common/components/classes";
import { Text } from "~/renderer/common/components/text";
import { AppToasts } from "~/renderer/common/components/toast";
import { cn } from "~/renderer/common/components/utils";
import { useWindowIsFullScreen } from "~/renderer/common/hooks/use-window";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

interface WindowProps {
  /** Title shown in the custom title bar; the page title by default. */
  title?: string;

  children: ReactNode;
}

export function Window({ title, children }: Readonly<WindowProps>) {
  const isFullScreen = useWindowIsFullScreen();

  return (
    <div className="flex h-screen w-screen flex-col bg-(--neutral-3) text-(--neutral-10)">
      {/* ---- Title bar. -------------------------------------------------- */}
      {isFullScreen || (
        <div
          className={cn(
            "title-bar-drag flex h-[30px] w-full shrink-0 items-center justify-center gap-1",
            chrome(),
          )}
        >
          <BlueTitIcon
            width={16}
            height={16}
            aria-label="BlueTit logo"
            role="img"
          />
          <Text size="2" weight="bold">
            {title ?? document.title}
          </Text>
        </div>
      )}

      {/* ---- Content. ---------------------------------------------------- */}
      <div className="min-h-0 grow">{children}</div>

      <AppToasts />
    </div>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
