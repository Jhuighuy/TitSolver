/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { Box, Flex } from "@radix-ui/themes";
import { useRef, useState } from "react";

import { Toolbar } from "~/renderer-help/components/toolbar";
import {
  type Navigation,
  type SearchResult,
  WebView,
  type WebViewHandle,
} from "~/renderer-help/components/webview";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

interface TabPaneProps {
  url: string;
  onNavigate: (url?: string) => void;
  onTitleChange: (title: string) => void;
  onOpenInNewTab: (url: string) => void;
}

export function TabPane({
  url,
  onOpenInNewTab,
  onNavigate,
  onTitleChange,
}: Readonly<TabPaneProps>) {
  const webViewRef = useRef<WebViewHandle | null>(null);

  // ---- Navigation. ----------------------------------------------------------

  const [navigation, setNavigation] = useState<Navigation>({
    url,
    state: "loading",
  });

  function goBack() {
    webViewRef.current?.goBack();
  }

  function goForward() {
    webViewRef.current?.goForward();
  }

  function goHome() {
    onNavigate();
  }

  function reload() {
    webViewRef.current?.reload();
  }

  function handleNavigationChange(navigation: Navigation) {
    setNavigation(navigation);
    onNavigate(navigation.url);
    if (navigation.title !== undefined) onTitleChange(navigation.title);
  }

  // ---- Search. --------------------------------------------------------------

  const [searchQuery, setSearchQuery] = useState("");
  const [searchResult, setSearchResult] = useState<SearchResult>();

  function searchNext() {
    webViewRef.current?.search(searchQuery, true);
  }

  function searchPrevious() {
    webViewRef.current?.search(searchQuery, false);
  }

  function handleSearchQueryChanged(query: string) {
    setSearchQuery(query);
    if (query.trim().length === 0) {
      setSearchResult(undefined);
      webViewRef.current?.clearSearch();
    } else {
      webViewRef.current?.search(query);
    }
  }

  // ---- Layout. --------------------------------------------------------------

  return (
    <Flex direction="column" width="100%" height="100%">
      {/* ---- Toolbar. ---------------------------------------------------- */}
      <Toolbar
        navigation={navigation}
        searchQuery={searchQuery}
        searchResult={searchResult}
        onBack={goBack}
        onForward={goForward}
        onHome={goHome}
        onReload={reload}
        onSearchNext={searchNext}
        onSearchPrevious={searchPrevious}
        onSearchQueryChanged={handleSearchQueryChanged}
      />

      {/* ---- Web View. -------------------------------------------------- */}
      <Box asChild flexGrow="1">
        <WebView
          ref={webViewRef}
          url={url}
          onNavigationChange={handleNavigationChange}
          onOpenInNewTab={onOpenInNewTab}
          onSearchResult={setSearchResult}
        />
      </Box>
    </Flex>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
