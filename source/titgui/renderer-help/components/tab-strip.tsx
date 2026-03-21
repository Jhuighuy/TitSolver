/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { Flex, IconButton, Text } from "@radix-ui/themes";
import { cva } from "class-variance-authority";
import { TbX as CloseIcon, TbPlus as PlusIcon } from "react-icons/tb";

import { chrome, hoverSurface } from "~/renderer-common/components/classes";
import type { Tab, TabID, TabTitles } from "~/shared/help-session";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

interface TabStripProps {
  tabs: Tab[];
  tabTitles: TabTitles;
  activeTabID?: TabID;
  onAddTab: () => void;
  onCloseTab: (id: TabID) => void;
  onSelectTab: (id: TabID) => void;
}

export function TabStrip({
  tabs,
  tabTitles,
  activeTabID,
  onAddTab,
  onCloseTab,
  onSelectTab,
}: Readonly<TabStripProps>) {
  return (
    <Flex
      align="end"
      gap="1"
      height="32px"
      px="1"
      pt="1"
      className={chrome({ direction: "bl" })}
    >
      <Flex align="end" gap="2px" flexGrow="1" minWidth="0">
        {/* ---- Opened Tabs. ---------------------------------------------- */}
        {tabs.map(({ id }) => (
          <Flex
            key={id}
            align="center"
            gap="2"
            px="3"
            py="2"
            minWidth="0px"
            maxWidth="280px"
            height="28px"
            onClick={() => {
              onSelectTab(id);
            }}
            onMouseDown={(event) => {
              if (event.button === 1) {
                event.preventDefault();
                onCloseTab(id);
              }
            }}
            className={tabStripItem({ active: id === activeTabID })}
          >
            <Text size="1" truncate>
              {tabTitles[id]}
            </Text>

            <IconButton
              variant="ghost"
              size="1"
              color="gray"
              onClick={(event) => {
                event.stopPropagation();
                onCloseTab(id);
              }}
            >
              <CloseIcon size={12} />
            </IconButton>
          </Flex>
        ))}

        {/* ---- New tab. -------------------------------------------------- */}
        <Flex
          align="center"
          justify="center"
          px="3"
          py="2"
          width="32px"
          height="28px"
          onClick={() => {
            onAddTab();
          }}
          className={tabStripItem()}
        >
          <IconButton variant="ghost" size="1" color="gray">
            <PlusIcon size={12} />
          </IconButton>
        </Flex>
      </Flex>
    </Flex>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

const tabStripItem = cva(
  [
    "rounded-t-lg border border-b-0 transition-colors",
    "light:border-slate-300/70 light:bg-slate-200/85",
    "dark:border-slate-700/50 dark:bg-slate-800/85",
  ],
  {
    variants: {
      active: {
        false: hoverSurface(),
        true: [
          "light:border-slate-300/90 light:bg-slate-300",
          "dark:border-slate-700/60 dark:bg-slate-900",
        ],
      },
      interactive: {
        false: "",
        true: "cursor-pointer",
      },
    },
    defaultVariants: {
      active: false,
      interactive: true,
    },
  },
);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
