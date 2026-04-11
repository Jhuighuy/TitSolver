/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import {
  IconArrowBackUp,
  IconArrowForwardUp,
  IconChevronDown,
  IconChevronUp,
  IconHome,
  IconReload,
  IconSearch,
  IconWorld,
  IconX,
} from "@tabler/icons-react";

import { IconButton } from "~/renderer-common/components/button";
import { chrome, surface } from "~/renderer-common/components/classes";
import { TextInput } from "~/renderer-common/components/input";
import { Flex } from "~/renderer-common/components/layout";
import { Separator } from "~/renderer-common/components/separator";
import { Spinner } from "~/renderer-common/components/spinner";
import { Text } from "~/renderer-common/components/text";
import { cn } from "~/renderer-common/components/utils";
import type {
  Navigation,
  SearchResult,
} from "~/renderer-help/components/webview";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

interface ToolbarProps {
  navigation: Navigation;
  searchQuery: string;
  searchResult?: SearchResult;
  onBack: () => void;
  onForward: () => void;
  onHome: () => void;
  onReload: () => void;
  onSearchNext: () => void;
  onSearchPrevious: () => void;
  onSearchQueryChanged: (query: string) => void;
}

export function Toolbar({
  navigation,
  searchQuery,
  searchResult,
  onBack,
  onForward,
  onHome,
  onReload,
  onSearchNext,
  onSearchPrevious,
  onSearchQueryChanged,
}: Readonly<ToolbarProps>) {
  // ---- Layout. --------------------------------------------------------------

  const { url, state, canGoBack, canGoForward } = navigation;

  return (
    <Flex
      align="center"
      height="9"
      minHeight="9"
      maxHeight="9"
      px="2"
      gap="3"
      className={chrome()}
    >
      {/* ---- Back / Forward. --------------------------------------------- */}
      <IconButton size="2" disabled={!canGoBack} onClick={onBack}>
        <IconArrowBackUp />
      </IconButton>

      <IconButton size="2" disabled={!canGoForward} onClick={onForward}>
        <IconArrowForwardUp />
      </IconButton>

      <Separator orientation="vertical" />

      {/* ---- Reload. ----------------------------------------------------- */}
      <IconButton size="2" onClick={onReload}>
        <IconReload />
      </IconButton>

      <Separator orientation="vertical" />

      {/* ---- Home. ------------------------------------------------------- */}
      <IconButton size="2" onClick={onHome}>
        <IconHome />
      </IconButton>

      <Separator orientation="vertical" />

      {/* ---- URL. -------------------------------------------------------- */}
      <Flex
        flexGrow="1"
        align="center"
        gap="2"
        px="3"
        py="1"
        className={cn(surface(), "rounded-full border [&_svg]:shrink-0")}
      >
        {state === "loading" ? <Spinner /> : <IconWorld className="size-3.5" />}
        <Text color="subtle" truncate className="min-w-0 flex-1">
          {url}
        </Text>
      </Flex>

      <Separator orientation="vertical" />

      {/* ---- Search. ----------------------------------------------------- */}
      <TextInput
        value={searchQuery}
        radius="full"
        placeholder="Find in page"
        slot={<IconSearch />}
        onValueChange={(value) => {
          onSearchQueryChanged(value);
        }}
        onKeyDown={(event) => {
          if (event.key === "Enter") {
            event.preventDefault();
            if (event.shiftKey) onSearchPrevious();
            else onSearchNext();
          }
          if (event.key === "Escape") {
            event.preventDefault();
            onSearchQueryChanged("");
          }
        }}
        className={
          searchQuery.trim() === ""
            ? undefined
            : "[&_input]:pr-14 [&_input]:sm:pr-22"
        }
      >
        {searchQuery.trim() !== "" && (
          <>
            {searchResult && (
              <Text color="subtle" className="hidden sm:block">
                {`${searchResult.activeMatch}/${searchResult.matches}`}
              </Text>
            )}
            <IconButton
              aria-label="Clear search"
              className="pointer-events-auto"
              onClick={() => {
                onSearchQueryChanged("");
              }}
            >
              <IconX />
            </IconButton>
          </>
        )}
      </TextInput>

      {searchQuery.trim() !== "" && (
        <>
          <IconButton size="2" onClick={onSearchPrevious}>
            <IconChevronUp />
          </IconButton>

          <IconButton size="2" onClick={onSearchNext}>
            <IconChevronDown />
          </IconButton>
        </>
      )}
    </Flex>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
