/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { useState } from "react";

import { Tabs } from "~/renderer-common/components/tabs";
import { TabPane } from "~/renderer-help/components/tab-pane";
import { useTabs } from "~/renderer-help/hooks/use-tabs";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export function Shell() {
  // ---- Tabs. ----------------------------------------------------------------

  const { activeTabID, tabs, addTab, closeTab, selectTab, navigateTab } =
    useTabs();

  const [titles, setTitles] = useState<Record<number, string>>({});

  function setTitleByID(id: number, title: string) {
    setTitles((previous) => ({ ...previous, [id]: title }));
  }

  // ---- Layout. --------------------------------------------------------------

  return (
    <Tabs
      value={activeTabID ?? null}
      onValueChange={(value) => {
        if (typeof value === "number") selectTab(value);
      }}
      onAddTab={addTab}
      onCloseTab={(value) => {
        if (typeof value === "number") closeTab(value);
      }}
    >
      <Tabs.List>
        {tabs.map((tab) => (
          <Tabs.Tab key={tab.id} value={tab.id}>
            {titles[tab.id]}
          </Tabs.Tab>
        ))}
      </Tabs.List>

      {tabs.map((tab) => (
        <Tabs.Panel key={tab.id} value={tab.id} keepMounted={true}>
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
        </Tabs.Panel>
      ))}
    </Tabs>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
