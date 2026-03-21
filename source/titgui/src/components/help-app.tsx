/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { Box, Flex, IconButton, Separator, Text } from "@radix-ui/themes";
import { useEffect, useRef, useState } from "react";
import {
  TbArrowBackUp as BackIcon,
  TbArrowForwardUp as ForwardIcon,
  TbHome as HomeIcon,
  TbReload as ReloadIcon,
} from "react-icons/tb";

import { chrome, surface } from "~/components/classes";
import { Window } from "~/components/window";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

type NavigationState = {
  canGoBack: boolean;
  canGoForward: boolean;
  title: string;
  url: string;
};

const HELP_TITLE = "BlueTit Manual";

export function HelpApp() {
  const manualUrl = new URL(window.location.href).searchParams.get("manualUrl");
  const [title, setTitle] = useState(HELP_TITLE);

  return (
    <Window title={title}>
      {manualUrl === null || manualUrl === "" ? (
        <MissingManual setTitle={setTitle} />
      ) : (
        <HelpPage manualUrl={manualUrl} setTitle={setTitle} />
      )}
    </Window>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

function MissingManual({
  setTitle,
}: Readonly<{ setTitle: (title: string) => void }>) {
  useEffect(() => {
    document.title = HELP_TITLE;
    setTitle(HELP_TITLE);
  }, [setTitle]);

  return (
    <Flex
      align="center"
      justify="center"
      flexGrow="1"
      p="6"
      className={surface()}
    >
      <Text size="2">Manual URL is not available.</Text>
    </Flex>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

function HelpPage({
  manualUrl,
  setTitle,
}: Readonly<{
  manualUrl: string;
  setTitle: (title: string) => void;
}>) {
  const webviewRef = useRef<WebViewElement | null>(null);
  const [isReady, setIsReady] = useState(false);
  const [state, setState] = useState<NavigationState>({
    canGoBack: false,
    canGoForward: false,
    title: HELP_TITLE,
    url: manualUrl,
  });

  useEffect(() => {
    document.title = state.title;
    setTitle(state.title);
  }, [setTitle, state.title]);

  useEffect(() => {
    const webview = webviewRef.current;
    if (webview === null) return;

    const syncState = () => {
      if (!isReady) return;

      const title = webview.getTitle().trim();
      setState({
        canGoBack: webview.canGoBack(),
        canGoForward: webview.canGoForward(),
        title: title === "" ? HELP_TITLE : title,
        url: webview.getURL() || manualUrl,
      });
    };

    const events = [
      "did-navigate",
      "did-navigate-in-page",
      "did-stop-loading",
      "page-title-updated",
    ] as const;

    const handleDomReady = () => {
      setIsReady(true);
    };

    webview.addEventListener("dom-ready", handleDomReady);
    for (const eventName of events) {
      webview.addEventListener(eventName, syncState);
    }

    return () => {
      webview.removeEventListener("dom-ready", handleDomReady);
      for (const eventName of events) {
        webview.removeEventListener(eventName, syncState as EventListener);
      }
    };
  }, [isReady, manualUrl]);

  useEffect(() => {
    if (!isReady) return;

    const webview = webviewRef.current;
    if (webview === null) return;

    const title = webview.getTitle().trim();
    setState({
      canGoBack: webview.canGoBack(),
      canGoForward: webview.canGoForward(),
      title: title === "" ? HELP_TITLE : title,
      url: webview.getURL() || manualUrl,
    });
  }, [isReady, manualUrl]);

  return (
    <Flex direction="column" flexGrow="1" minHeight="0" gap="1px">
      <NavBar
        title={state.title}
        canGoBack={state.canGoBack}
        canGoForward={state.canGoForward}
        onBack={() => webviewRef.current?.goBack()}
        onForward={() => webviewRef.current?.goForward()}
        onReload={() => webviewRef.current?.reload()}
        onHome={() => void webviewRef.current?.loadURL(manualUrl)}
      />

      <Box
        flexGrow="1"
        minHeight="0"
        className={surface()}
        style={{ display: "flex" }}
      >
        <webview
          ref={webviewRef}
          src={manualUrl}
          style={{
            display: "inline-flex",
            flex: 1,
            width: "100%",
            height: "100%",
          }}
        />
      </Box>
    </Flex>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

type NavBarProps = {
  title: string;
  canGoBack: boolean;
  canGoForward: boolean;
  onBack: () => void;
  onForward: () => void;
  onReload: () => void;
  onHome: () => void;
};

function NavBar({
  title,
  canGoBack,
  canGoForward,
  onBack,
  onForward,
  onReload,
  onHome,
}: Readonly<NavBarProps>) {
  return (
    <Flex
      direction="row"
      align="center"
      height="36px"
      px="4"
      py="4px"
      gap="3"
      className={chrome({ direction: "br" })}
    >
      <IconButton variant="ghost" disabled={!canGoBack} onClick={onBack}>
        <BackIcon size={16} />
      </IconButton>

      <IconButton variant="ghost" disabled={!canGoForward} onClick={onForward}>
        <ForwardIcon size={16} />
      </IconButton>

      <Separator orientation="vertical" size="1" />

      <IconButton variant="ghost" onClick={onReload}>
        <ReloadIcon size={16} />
      </IconButton>

      <Separator orientation="vertical" size="1" />

      <IconButton variant="ghost" onClick={onHome}>
        <HomeIcon size={16} />
      </IconButton>

      <Separator orientation="vertical" size="1" />

      <Flex direction="column" minWidth="0" flexGrow="1">
        <Text size="1" weight="bold" truncate>
          {title}
        </Text>
      </Flex>
    </Flex>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
