/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import {
  app,
  BrowserWindow,
  clipboard,
  Menu,
  MenuItemConstructorOptions,
  shell,
} from "electron";
import path from "node:path";
import { fileURLToPath, pathToFileURL } from "node:url";
import z from "zod";

import { Installation } from "~/main/installation";
import { WindowController } from "~/main/window";
import {
  HELP_SESSION_CHANGED_CHANNEL,
  WEBVIEW_OPEN_IN_TAB_CHANNEL,
} from "~/shared/channels";
import type { HelpSession, Tab, TabID } from "~/shared/help-session";
import { assert } from "~/shared/utils";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
 * Help manager.
 */
export class HelpManager {
  private readonly manualPath: string;
  private readonly homeURL: string;
  private readonly sessionModel: HelpSessionModel;

  /**
   * Construct a help manager.
   */
  public constructor(
    install: Installation,
    public readonly controller: WindowController,
  ) {
    // Initialize paths.
    this.manualPath = install.manualPath;
    this.homeURL = this.pathToURL("index.html");

    // Restore session from persisted state.
    const sessionModelSchema = z
      .object({
        activeTabID: z.number().optional(),
        tabs: z.array(z.object({ id: z.number(), path: z.string() })),
      })
      .transform(({ activeTabID, tabs }, context) => {
        try {
          return new HelpSessionModel(
            activeTabID,
            tabs.map(({ id, path }) => ({ id, url: this.pathToURL(path) })),
          );
        } catch (error) {
          context.addIssue({
            code: "custom",
            message: String(error),
            path: [],
          });
          return new HelpSessionModel();
        }
      });
    this.sessionModel = this.controller.persist.get(
      HELP_SESSION_KEY,
      sessionModelSchema,
      new HelpSessionModel(),
    );

    // Setup webview.
    app.on("web-contents-created", (_event, contents) => {
      switch (contents.getType()) {
        case "window": {
          // Once webview is attached, configure web preferences.
          contents.on("will-attach-webview", (event, webPreferences) => {
            const window = BrowserWindow.fromWebContents(contents);
            if (window !== this.controller.window) {
              event.preventDefault();
              return;
            }

            webPreferences.preload = path.join(__dirname, "webview-preload.js");
            webPreferences.nodeIntegration = false;
            webPreferences.contextIsolation = true;
            webPreferences.sandbox = true;
            webPreferences.webSecurity = true;
            Reflect.deleteProperty(webPreferences, "preloadURL");
          });
          break;
        }
        case "webview": {
          // Handle window open requests. External links are opened externally.
          contents.setWindowOpenHandler(({ url }) => {
            if (this.isManualURL(url)) {
              contents.send(WEBVIEW_OPEN_IN_TAB_CHANNEL, url);
            } else {
              void shell.openExternal(url);
            }
            return { action: "deny" };
          });

          // Handle navigations. External links are opened externally.
          contents.on("will-navigate", (event, url) => {
            if (!this.isManualURL(url)) {
              event.preventDefault();
              void shell.openExternal(url);
            }
          });

          // Handle context menu.
          contents.on("context-menu", (_event, params) => {
            const menuItems: MenuItemConstructorOptions[] = [];

            const url = params.linkURL;
            if (url.trim() === "") {
              if (params.isEditable) {
                menuItems.push(
                  {
                    label: "Undo",
                    enabled: params.editFlags.canUndo,
                    click: () => {
                      contents.undo();
                    },
                  },
                  {
                    label: "Redo",
                    enabled: params.editFlags.canRedo,
                    click: () => {
                      contents.redo();
                    },
                  },
                  { type: "separator" },
                  {
                    label: "Cut",
                    enabled: params.editFlags.canCut,
                    click: () => {
                      contents.cut();
                    },
                  },
                  {
                    label: "Copy",
                    enabled: params.editFlags.canCopy,
                    click: () => {
                      contents.copy();
                    },
                  },
                  {
                    label: "Paste",
                    enabled: params.editFlags.canPaste,
                    click: () => {
                      contents.paste();
                    },
                  },
                  {
                    label: "Delete",
                    enabled: params.editFlags.canDelete,
                    click: () => {
                      contents.delete();
                    },
                  },
                  {
                    label: "Select All",
                    enabled: params.editFlags.canSelectAll,
                    click: () => {
                      contents.selectAll();
                    },
                  },
                );
              } else {
                menuItems.push(
                  {
                    label: "Back",
                    enabled: contents.navigationHistory.canGoBack(),
                    click: () => {
                      contents.navigationHistory.goBack();
                    },
                  },
                  {
                    label: "Forward",
                    enabled: contents.navigationHistory.canGoForward(),
                    click: () => {
                      contents.navigationHistory.goForward();
                    },
                  },
                  {
                    label: "Reload",
                    click: () => {
                      contents.reload();
                    },
                  },
                  { type: "separator" },
                  {
                    label: "Copy",
                    enabled: params.editFlags.canCopy,
                    click: () => {
                      contents.copy();
                    },
                  },
                  { type: "separator" },
                );
              }
            } else {
              if (this.isManualURL(url)) {
                menuItems.push(
                  {
                    label: "Open Link",
                    click: () => {
                      void contents.loadURL(url);
                    },
                  },
                  {
                    label: "Open Link in New Tab",
                    click: () => {
                      contents.send(WEBVIEW_OPEN_IN_TAB_CHANNEL, url);
                    },
                  },
                );
              } else {
                menuItems.push(
                  {
                    label: "Open Link Externally",
                    click: () => {
                      void shell.openExternal(url);
                    },
                  },
                  {
                    label: "Copy Link Address",
                    click: () => {
                      clipboard.clear();
                      clipboard.writeText(url);
                    },
                  },
                );
              }
            }
            const menu = Menu.buildFromTemplate(menuItems);
            menu.popup({ x: params.x, y: params.y });
          });
          break;
        }
      }
    });
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /**
   * Get the current session.
   */
  public get session() {
    return this.sessionModel as Readonly<HelpSession>;
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /**
   * Add a new tab.
   */
  public addTab(url?: string) {
    // Add tab.
    assert(this.isManualURL(url));
    this.sessionModel.addTab(url ?? this.homeURL);

    // Notify the listeners.
    this.sessionChanged();

    // Focus the window.
    this.controller.open();
    this.controller.window?.focus();
  }

  /**
   * Reveal the existing tab with the given URL, or add a new tab.
   */
  public revealTab(url?: string) {
    // Reveal tab.
    assert(this.isManualURL(url));
    this.sessionModel.revealTab(url ?? this.homeURL);

    // Notify the listeners.
    this.sessionChanged();

    // Focus the window.
    this.controller.open();
    this.controller.window?.focus();
  }

  /**
   * Close the tab with the given ID.
   */
  public closeTab(id: number) {
    // Close tab.
    this.sessionModel.closeTab(id);

    // Notify the listeners.
    this.sessionChanged();

    // If there are no more tabs, close the window.
    if (this.sessionModel.tabs.length === 0) this.controller.close();
  }

  /**
   * Select the tab with the given ID.
   */
  public selectTab(id: number) {
    // Select tab.
    this.sessionModel.selectTab(id);

    // Notify the listeners.
    this.sessionChanged();
  }

  /**
   * Update the URL of the tab with the given ID.
   */
  public navigateTab(id: number, url?: string) {
    // Update tab.
    assert(this.isManualURL(url));
    this.sessionModel.navigateTab(id, url ?? this.homeURL);

    // Notify the listeners.
    this.sessionChanged();
  }

  // Notify the listeners about the session change.
  private sessionChanged() {
    // Notify the renderer process.
    this.controller.window?.webContents.send(HELP_SESSION_CHANGED_CHANNEL, {
      activeTabID: this.sessionModel.activeTabID,
      tabs: this.sessionModel.tabs,
    });

    // Persist the session.
    this.controller.persist.set(HELP_SESSION_KEY, {
      activeTabID: this.sessionModel.activeTabID,
      tabs: this.sessionModel.tabs.map(({ id, url }) => ({
        id,
        path: this.urlToPath(url),
      })),
    });
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  // Convert a manual-relative path to a help URL.
  private pathToURL(relativePath: string) {
    try {
      assert(relativePath.trim() !== "");
      assert(!path.isAbsolute(relativePath));

      const resolvedPath = path.resolve(this.manualPath, relativePath);
      const normalizedPath = path.relative(this.manualPath, resolvedPath);
      assert(normalizedPath !== "");
      assert(!normalizedPath.startsWith(".."));
      assert(!path.isAbsolute(normalizedPath));

      return pathToFileURL(resolvedPath).toString();
    } catch {
      throw new Error(`Invalid manual-relative path: '${relativePath}'.`);
    }
  }

  // Convert a help URL to a manual-relative path.
  private urlToPath(url: string) {
    try {
      const parsedURL = new URL(url, this.homeURL);
      assert(parsedURL.protocol === "file:");

      const resolvedPath = path.resolve(fileURLToPath(parsedURL));
      const relativePath = path.relative(this.manualPath, resolvedPath);
      assert(relativePath !== "");
      assert(!relativePath.startsWith(".."));
      assert(!path.isAbsolute(relativePath));

      return relativePath;
    } catch {
      throw new Error(`Invalid help URL: '${url}'.`);
    }
  }

  // Check if the given URL belongs to the manual.
  private isManualURL(url?: string) {
    if (url === undefined) return true;
    try {
      this.urlToPath(url);
      return true;
    } catch {
      return false;
    }
  }
}

const HELP_SESSION_KEY = "help.session";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
 * Help session model.
 */
class HelpSessionModel implements HelpSession {
  /**
   * Active tab ID.
   */
  public activeTabID: TabID | undefined;

  /**
   * Array of tabs.
   */
  public tabs: Tab[];

  /**
   * Construct help session model.
   */
  public constructor(activeTabID?: TabID, tabs: Tab[] = []) {
    const tabIDs = new Set<TabID>();
    for (const tab of tabs) {
      assert(!tabIDs.has(tab.id), `Duplicate tab ID: ${tab.id}.`);
      tabIDs.add(tab.id);
    }

    if (tabs.length === 0) {
      assert(
        activeTabID === undefined,
        "Active tab ID must be undefined when there are no tabs.",
      );
    } else {
      assert(
        tabs.some((tab) => tab.id === activeTabID),
        `Active tab ID ${activeTabID} does not exist in tabs.`,
      );
    }

    this.activeTabID = activeTabID;
    this.tabs = tabs.map((tab) => ({ ...tab }));
  }

  /**
   * Find the tab with the given ID.
   */
  public findTab(id: TabID) {
    const index = this.tabs.findIndex((tab) => tab.id === id);
    return [index === -1 ? undefined : this.tabs[index], index] as const;
  }

  /**
   * Add a new tab to the session.
   */
  public addTab(url: string) {
    const id = this.tabs.reduce((id, tab) => Math.max(id, tab.id), 0) + 1;
    this.tabs.push({ id, url });
    this.activeTabID = id;
  }

  /**
   * Reveal the existing tab with the given URL, or add a new tab.
   */
  public revealTab(url: string) {
    const tab = this.tabs.find((tab) => tab.url === url);
    if (tab === undefined) {
      this.addTab(url);
    } else {
      this.selectTab(tab.id);
    }
  }

  /**
   * Close the tab with the given ID.
   */
  public closeTab(id: TabID) {
    const [tab, index] = this.findTab(id);
    assert(tab !== undefined);
    this.tabs.splice(index, 1);
    if (this.activeTabID === id) {
      this.activeTabID =
        this.tabs.length === 0
          ? undefined
          : this.tabs[Math.max(0, index - 1)].id;
    }
  }

  /**
   * Select the tab with the given ID.
   */
  public selectTab(id: TabID) {
    const [tab] = this.findTab(id);
    assert(tab !== undefined);
    this.activeTabID = id;
  }

  /**
   * Update the URL of the tab with the given ID.
   */
  public navigateTab(id: TabID, url: string) {
    const [tab] = this.findTab(id);
    assert(tab !== undefined);
    tab.url = url;
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
