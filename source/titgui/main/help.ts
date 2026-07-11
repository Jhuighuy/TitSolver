/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import path from "node:path";
import { pathToFileURL } from "node:url";

import {
  app,
  BrowserWindow,
  clipboard,
  Menu,
  type MenuItemConstructorOptions,
  net,
  protocol,
  shell,
} from "electron";

import { HELP_PROTOCOL, HOME_URL, resolveHelpPath } from "~/main/help-path";
import type { Installation } from "~/main/installation";
import { sendIpcEvent } from "~/main/ipc";
import type { WindowController } from "~/main/window";
import {
  type HelpSession,
  type HelpSessionListener,
  helpSessionSchema,
  type HelpTab,
} from "~/shared/help";
import { assert } from "~/shared/utils";
import { WEBVIEW_OPEN_IN_TAB_CHANNEL } from "~/shared/webview";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
 * Help service.
 */
export class HelpService {
  private readonly protocol: HelpProtocol;
  private readonly sessionModel: HelpSessionModel;
  private readonly sessionListeners: Set<HelpSessionListener> = new Set();

  /** Construct a help service. */
  public constructor(
    install: Installation,
    public readonly controller: WindowController,
  ) {
    this.protocol = new HelpProtocol(install.manualPath);

    this.sessionModel = new HelpSessionModel(
      this.controller.persist.get("session", helpSessionSchema, {
        activeTabID: undefined,
        tabs: [],
      }),
    );
    this.onSessionChanged((session) => {
      this.controller.persist.set("session", session);
    });
    this.onSessionChanged((session) => {
      const window = this.controller.window;
      if (window !== undefined) {
        sendIpcEvent(window, "help", "sessionChanged", session);
      }
    });

    this.setupWebviewEventListeners();
  }

  /** Get the current session. */
  public getSession() {
    return this.sessionModel.session;
  }

  /** Add a new tab. */
  public addTab(url: string = HOME_URL) {
    // External links are opened externally.
    if (this.protocol.isExternalUrl(url)) {
      void shell.openExternal(url);
      return;
    }

    this.sessionModel.addTab(url);
    this.sessionChanged();

    // New tab focues the help window.
    this.controller.open();
    this.controller.window?.focus();
  }

  /**
   * Close the tab with the given ID.
   */
  public closeTab(id: number) {
    this.sessionModel.closeTab(id);
    this.sessionChanged();

    // When the last tab is closed, close the help window.
    if (this.sessionModel.isEmpty) this.controller.close();
  }

  /** Select the tab with the given ID. */
  public selectTab(id: number) {
    this.sessionModel.selectTab(id);
    this.sessionChanged();
  }

  /** Update the URL of the tab with the given ID. */
  public navigateTab(id: number, url: string = HOME_URL) {
    // External links are opened externally.
    if (this.protocol.isExternalUrl(url)) {
      void shell.openExternal(url);
      return;
    }

    this.sessionModel.navigateTab(id, url);
    this.sessionChanged();
  }

  /** Subscribe to session changes. */
  public onSessionChanged(listener: HelpSessionListener): () => void {
    this.sessionListeners.add(listener);
    return () => {
      this.sessionListeners.delete(listener);
    };
  }

  // Notify the listeners about the session change.
  private sessionChanged() {
    for (const listener of this.sessionListeners) {
      listener(this.sessionModel.session);
    }
  }

  // Setup event listeners for webview contents.
  private setupWebviewEventListeners() {
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
            if (this.protocol.isExternalUrl(url)) {
              void shell.openExternal(url);
            } else {
              contents.send(WEBVIEW_OPEN_IN_TAB_CHANNEL, url);
            }
            return { action: "deny" };
          });

          // Handle navigations. External links are opened externally.
          contents.on("will-navigate", (event, url) => {
            if (this.protocol.isExternalUrl(url)) {
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
            } else if (this.protocol.isExternalUrl(url)) {
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
            const menu = Menu.buildFromTemplate(menuItems);
            menu.popup({ x: params.x, y: params.y });
          });
          break;
        }
        case "backgroundPage":
        case "browserView":
        case "offscreen":
        case "remote":
          // Do nothing.
          break;
      }
    });
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
 * Help link protocol implementation.
 */
class HelpProtocol {
  /** Construct a help URL protocol. */
  public constructor(private readonly rootPath: string) {
    protocol.handle(HELP_PROTOCOL, async ({ url }) => {
      try {
        const filePath = resolveHelpPath(this.rootPath, url);
        return await net.fetch(pathToFileURL(filePath).toString());
      } catch {
        const notFoundPath = path.join(this.rootPath, "404.html");
        return net.fetch(pathToFileURL(notFoundPath).toString());
      }
    });
  }

  /** Check if the given URL does not belong to the manual. */
  public isExternalUrl(url: string) {
    try {
      resolveHelpPath(this.rootPath, url);
      return false;
    } catch {
      return true;
    }
  }
}

protocol.registerSchemesAsPrivileged([
  {
    scheme: HELP_PROTOCOL,
    privileges: {
      corsEnabled: true,
      secure: true,
      standard: true,
      supportFetchAPI: true,
    },
  },
]);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
 * Help session model.
 */
class HelpSessionModel {
  private activeTabID?: number;
  private readonly tabs: HelpTab[] = [];

  /** Construct help session model. */
  public constructor(session: HelpSession) {
    this.activeTabID = session.activeTabID;
    this.tabs = session.tabs;
  }

  /** Get the current session. */
  public get session() {
    return { activeTabID: this.activeTabID, tabs: this.tabs };
  }

  /** Check if session is empty. */
  public get isEmpty() {
    return this.tabs.length === 0;
  }

  /** Find the tab with the given ID. */
  public findTab(id: number) {
    const index = this.tabs.findIndex((tab) => tab.id === id);
    return [index === -1 ? undefined : this.tabs[index], index] as const;
  }

  /** Add a new tab to the session. */
  public addTab(url: string) {
    const id = this.tabs.reduce((id, tab) => Math.max(id, tab.id), 0) + 1;
    this.tabs.push({ id, url });
    this.activeTabID = id;
  }

  /** Close the tab with the given ID. */
  public closeTab(id: number) {
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

  /** Select the tab with the given ID. */
  public selectTab(id: number) {
    const [tab] = this.findTab(id);
    assert(tab !== undefined);
    this.activeTabID = id;
  }

  /** Update the URL of the tab with the given ID. */
  public navigateTab(id: number, url: string) {
    const [tab] = this.findTab(id);
    assert(tab !== undefined);
    tab.url = url;
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
