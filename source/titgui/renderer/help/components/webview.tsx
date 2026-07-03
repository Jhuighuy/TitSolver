/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import type { FoundInPageEvent, IpcMessageEvent, WebviewTag } from "electron";
import {
  useEffect,
  useEffectEvent,
  useImperativeHandle,
  useState,
} from "react";

import { WEBVIEW_OPEN_IN_TAB_CHANNEL } from "~/shared/channels";
import { assert } from "~/shared/utils";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export interface Navigation {
  state: "loading" | "ready";
  url: string;
  title?: string;
  canGoBack?: boolean;
  canGoForward?: boolean;
}

export interface SearchResult {
  activeMatchOrdinal: number;
  matches: number;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export interface WebViewHandle {
  goBack: () => void;
  goForward: () => void;
  reload: () => void;
  search: (query: string, forward?: boolean) => void;
  clearSearch: () => void;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

interface WebViewProps {
  ref?: React.Ref<WebViewHandle>;
  url: string;
  onNavigationChange: (navigation: Navigation) => void;
  onOpenInNewTab: (url: string) => void;
  onSearchResult: (result: SearchResult) => void;
}

export function WebView({
  ref,
  url,
  onNavigationChange,
  onOpenInNewTab,
  onSearchResult,
}: Readonly<WebViewProps>) {
  // ---- Handle. --------------------------------------------------------------

  const [webview, setWebview] = useState<WebviewTag | null>(null);

  useImperativeHandle(
    ref,
    () => ({
      goBack: () => webview?.goBack(),
      goForward: () => webview?.goForward(),
      reload: () => webview?.reload(),
      search: (query, forward = true) =>
        webview?.findInPage(query, { forward, findNext: true }),
      clearSearch: () => webview?.stopFindInPage("clearSelection"),
    }),
    [webview],
  );

  // ---- Mount. ---------------------------------------------------------------

  const handleStartLoad = useEffectEvent(() => {
    onNavigationChange({
      state: "loading",
      url,
    });
  });

  const handleNavigation = useEffectEvent(() => {
    if (webview === null) return;
    onNavigationChange({
      state: "ready",
      url: webview.getURL(),
      title: webview.getTitle(),
      canGoBack: webview.canGoBack(),
      canGoForward: webview.canGoForward(),
    });
  });

  const handleFoundInPage = useEffectEvent((event: FoundInPageEvent) => {
    const { activeMatchOrdinal, matches, finalUpdate } = event.result;
    if (finalUpdate) onSearchResult({ activeMatchOrdinal, matches });
  });

  const handleIPCMessage = useEffectEvent((event: IpcMessageEvent) => {
    if (event.channel !== WEBVIEW_OPEN_IN_TAB_CHANNEL) return;
    assert(event.args.length === 1 && typeof event.args[0] === "string");
    onOpenInNewTab(event.args[0]);
  });

  useEffect(() => {
    if (webview === null) return;

    const controller = new AbortController();

    /**
     * @todo Electron typing currently define the last parameter of as boolean,
     *       but not the `AddEventListenerOptions` type. Therefore, we have to
     *       make these `as unknown as boolean` casts.
     */
    webview.addEventListener("did-start-loading", handleStartLoad, {
      signal: controller.signal,
    } as unknown as boolean);

    webview.addEventListener("dom-ready", handleNavigation, {
      signal: controller.signal,
    } as unknown as boolean);

    webview.addEventListener("did-navigate", handleNavigation, {
      signal: controller.signal,
    } as unknown as boolean);

    webview.addEventListener("did-navigate-in-page", handleNavigation, {
      signal: controller.signal,
    } as unknown as boolean);

    webview.addEventListener("did-stop-loading", handleNavigation, {
      signal: controller.signal,
    } as unknown as boolean);

    webview.addEventListener("page-title-updated", handleNavigation, {
      signal: controller.signal,
    } as unknown as boolean);

    webview.addEventListener("found-in-page", handleFoundInPage, {
      signal: controller.signal,
    } as unknown as boolean);

    webview.addEventListener("ipc-message", handleIPCMessage, {
      signal: controller.signal,
    } as unknown as boolean);

    return () => {
      controller.abort();
    };
  }, [webview]);

  // Assign `src` only after the ref callback has attached all listeners.
  // Web view can emit its first loading/title events immediately, so wiring
  // listeners first keeps the initial navigation observable.
  useEffect(() => {
    if (webview === null || webview.src === url) return;
    webview.setAttribute("src", url);
  }, [url, webview]);

  // ---- Layout. --------------------------------------------------------------

  return (
    <webview
      ref={(webview: WebviewTag | null) => {
        setWebview(webview);
      }}
      style={{ width: "100%", height: "100%" }}
    />
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
