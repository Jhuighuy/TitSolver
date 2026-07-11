/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { IconCube, IconHome, IconPlus } from "@tabler/icons-react";
import type { ReactNode } from "react";
import { z } from "zod";

import { Button } from "~/renderer/common/components/button";
import { chrome } from "~/renderer/common/components/classes";
import { DropdownMenu } from "~/renderer/common/components/dropdown-menu";
import { ErrorGuard } from "~/renderer/common/components/error-guard";
import { Tabs } from "~/renderer/common/components/tabs";
import { Text } from "~/renderer/common/components/text";
import { cn } from "~/renderer/common/components/utils";
import { useWindowState } from "~/renderer/common/hooks/use-window";
import { Timeline } from "~/renderer/main/components/timeline";
import { Viewport } from "~/renderer/main/components/viewport";
import { Welcome } from "~/renderer/main/components/welcome";
import {
  getViewportState,
  ViewportStateProvider,
} from "~/renderer/main/state/viewport";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Workspace tabs are typed instances. The welcome tab is single-instance
// (its ID is its kind); viewport tabs are numbered, and every viewport tab
// owns an independent state bundle keyed by its ID.
const workspaceTabSchema = z.object({
  id: z.string(),
  kind: z.union([z.literal("welcome"), z.literal("viewport")]),
});

type WorkspaceTab = z.infer<typeof workspaceTabSchema>;

const workspaceStateSchema = z.object({
  tabs: z.array(workspaceTabSchema),
  active: z.string().nullable(),
});

type WorkspaceState = z.infer<typeof workspaceStateSchema>;

const defaultWorkspaceState: WorkspaceState = {
  tabs: [
    { id: "welcome", kind: "welcome" },
    { id: "viewport-1", kind: "viewport" },
  ],
  active: "welcome",
};

const tabIcons: Record<WorkspaceTab["kind"], ReactNode> = {
  welcome: <IconHome />,
  viewport: <IconCube />,
};

function tabTitle(tab: WorkspaceTab, tabs: readonly WorkspaceTab[]) {
  if (tab.kind === "welcome") return "Welcome";
  const viewports = tabs.filter((other) => other.kind === "viewport");
  return viewports.length > 1
    ? `Viewport ${(viewports.indexOf(tab) + 1).toString()}`
    : "Viewport";
}

// The next free numbered viewport tab ID.
function nextViewportId(tabs: readonly WorkspaceTab[]) {
  const used = new Set(tabs.map((tab) => tab.id));
  for (let index = 1; ; index++) {
    const id = `viewport-${index.toString()}`;
    if (!used.has(id)) return id;
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
 * The tab workspace filling the center of the window.
 */
export function Workspace() {
  const [state, setState] = useWindowState(
    "workspace",
    workspaceStateSchema,
    defaultWorkspaceState,
  );

  const openWelcome = () => {
    setState((previous) => ({
      tabs: previous.tabs.some((tab) => tab.kind === "welcome")
        ? previous.tabs
        : [{ id: "welcome", kind: "welcome" as const }, ...previous.tabs],
      active: "welcome",
    }));
  };

  const openViewport = () => {
    setState((previous) => {
      const id = nextViewportId(previous.tabs);
      return {
        tabs: [...previous.tabs, { id, kind: "viewport" as const }],
        active: id,
      };
    });
  };

  // Activate the first viewport tab, creating one if none is open.
  const showViewport = () => {
    setState((previous) => {
      const viewport = previous.tabs.find((tab) => tab.kind === "viewport");
      if (viewport !== undefined) {
        return { ...previous, active: viewport.id };
      }
      const id = nextViewportId(previous.tabs);
      return {
        tabs: [...previous.tabs, { id, kind: "viewport" as const }],
        active: id,
      };
    });
  };

  const closeTab = (id: string) => {
    setState((previous) => {
      const tabs = previous.tabs.filter((tab) => tab.id !== id);
      return {
        tabs,
        active:
          previous.active === id ? (tabs.at(-1)?.id ?? null) : previous.active,
      };
    });
  };

  if (state.tabs.length === 0) {
    return (
      <EmptyWorkspace
        onOpenWelcome={openWelcome}
        onNewViewport={openViewport}
      />
    );
  }

  // Heal a persisted active tab that is no longer open.
  const active =
    state.active !== null && state.tabs.some((tab) => tab.id === state.active)
      ? state.active
      : (state.tabs.at(-1)?.id ?? null);

  const welcomeIsOpen = state.tabs.some((tab) => tab.kind === "welcome");

  return (
    <Tabs
      value={active}
      onValueChange={(id) => {
        setState((previous) => ({ ...previous, active: id }));
      }}
      onCloseTab={closeTab}
    >
      <Tabs.List>
        {state.tabs.map((tab) => (
          <Tabs.Tab key={tab.id} value={tab.id} icon={tabIcons[tab.kind]}>
            {tabTitle(tab, state.tabs)}
          </Tabs.Tab>
        ))}
        <AddTabMenu
          welcomeIsOpen={welcomeIsOpen}
          onOpenWelcome={openWelcome}
          onNewViewport={openViewport}
        />
      </Tabs.List>

      {state.tabs.map((tab) => (
        <Tabs.Panel
          key={tab.id}
          value={tab.id}
          // Viewports own WebGL contexts; keep them alive across switches.
          keepMounted={tab.kind === "viewport"}
        >
          <ErrorGuard>
            {tab.kind === "welcome" ? (
              <Welcome onCaseOpened={showViewport} />
            ) : (
              <ViewportPane id={tab.id} />
            )}
          </ErrorGuard>
        </Tabs.Panel>
      ))}
    </Tabs>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

interface AddTabMenuProps {
  welcomeIsOpen: boolean;
  onOpenWelcome: () => void;
  onNewViewport: () => void;
}

// The "+" control of the tab bar: a menu of openable tabs.
function AddTabMenu({
  welcomeIsOpen,
  onOpenWelcome,
  onNewViewport,
}: Readonly<AddTabMenuProps>) {
  return (
    <DropdownMenu.Root>
      <DropdownMenu.Trigger aria-label="Add Tab">
        <IconPlus />
      </DropdownMenu.Trigger>
      <DropdownMenu.Content>
        {!welcomeIsOpen && (
          <DropdownMenu.Item onClick={onOpenWelcome}>
            {tabIcons.welcome}
            Welcome
          </DropdownMenu.Item>
        )}
        <DropdownMenu.Item onClick={onNewViewport}>
          {tabIcons.viewport}
          New Viewport
        </DropdownMenu.Item>
      </DropdownMenu.Content>
    </DropdownMenu.Root>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// A viewport tab: the 3D view with its per-viewport controls and timeline.
// The state bundle is looked up by tab ID, so every viewport tab gets its
// own camera, field selection, and coloring.
function ViewportPane({ id }: Readonly<{ id: string }>) {
  return (
    <ViewportStateProvider value={getViewportState(id)}>
      <div className="flex size-full min-h-0 flex-col">
        <div className="min-h-0 grow">
          <ErrorGuard>
            <Viewport />
          </ErrorGuard>
        </div>
        <ErrorGuard>
          <Timeline />
        </ErrorGuard>
      </div>
    </ViewportStateProvider>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

interface EmptyWorkspaceProps {
  onOpenWelcome: () => void;
  onNewViewport: () => void;
}

// Quiet centered screen shown when every tab is closed, listing the tabs
// that can be opened.
function EmptyWorkspace({
  onOpenWelcome,
  onNewViewport,
}: Readonly<EmptyWorkspaceProps>) {
  return (
    <div
      className={cn(
        "flex size-full flex-col items-center justify-center gap-6",
        chrome(),
      )}
    >
      <Text color="subtle">All tabs are closed</Text>
      <div className="flex gap-2">
        <Button variant="outline" size="2" onClick={onOpenWelcome}>
          {tabIcons.welcome}
          Welcome
        </Button>
        <Button variant="outline" size="2" onClick={onNewViewport}>
          {tabIcons.viewport}
          Viewport
        </Button>
      </div>
    </div>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
