/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { Box, Flex } from "@radix-ui/themes";
import { useState } from "react";

import { TabPane } from "~/renderer-help/components/tab-pane";
import { TabStrip } from "~/renderer-help/components/tab-strip";
import { useTabs } from "~/renderer-help/hooks/use-tabs";
import type { TabID, TabTitles } from "~/shared/help-session";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export function Shell() {
  // ---- Tabs. ----------------------------------------------------------------

  const { activeTabID, tabs, addTab, closeTab, selectTab, navigateTab } =
    useTabs();

  const [titles, setTitles] = useState<TabTitles>({});

  function setTitleByID(id: TabID, title: string) {
    setTitles((previous) => ({ ...previous, [id]: title }));
  }

  // ---- Layout. --------------------------------------------------------------

  return (
    <Flex direction="column" gap="1px" width="100%" height="100%">
      {/* ---- Tab Strip. -------------------------------------------------- */}
      <TabStrip
        activeTabID={activeTabID}
        tabs={tabs}
        tabTitles={titles}
        onAddTab={() => {
          addTab();
        }}
        onCloseTab={closeTab}
        onSelectTab={selectTab}
      />

      {/* ---- Tab Panes. -------------------------------------------------- */}
      {tabs.map((tab) => (
        // Inactive tabs are intentionally hidden only visually. This is crucial
        // for the correct loading of tab titles during initial app startup.
        <Box
          key={tab.id}
          flexGrow="1"
          width="100%"
          height="100%"
          {...(tab.id !== activeTabID && { display: "none" })}
        >
          <TabPane
            url={tab.url}
            onNavigate={(url) => {
              navigateTab(tab.id, url);
            }}
            onTitleChange={(title) => {
              setTitleByID(tab.id, title);
            }}
            onOpenInNewTab={addTab}
          />
        </Box>
      ))}
    </Flex>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
