/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import {
  Flex,
  IconButton,
  Separator,
  Spinner,
  Text,
  TextField,
} from "@radix-ui/themes";
import { cva } from "class-variance-authority";
import {
  TbArrowBackUp as BackIcon,
  TbX as ClearIcon,
  TbChevronDown as FindNextIcon,
  TbChevronUp as FindPreviousIcon,
  TbArrowForwardUp as ForwardIcon,
  TbHome as HomeIcon,
  TbReload as ReloadIcon,
  TbSearch as SearchIcon,
  TbWorld as WorldIcon,
} from "react-icons/tb";

import { chrome } from "~/renderer-common/components/classes";
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
      direction="row"
      align="center"
      height="36px"
      px="2"
      gap="3"
      className={chrome({ direction: "br" })}
    >
      {/* ---- Back / Forward. --------------------------------------------- */}
      <IconButton variant="ghost" disabled={!canGoBack} onClick={onBack}>
        <BackIcon size={16} />
      </IconButton>

      <IconButton variant="ghost" disabled={!canGoForward} onClick={onForward}>
        <ForwardIcon size={16} />
      </IconButton>

      <Separator orientation="vertical" size="1" />

      {/* ---- Reload. ----------------------------------------------------- */}
      <IconButton variant="ghost" onClick={onReload}>
        <ReloadIcon size={16} />
      </IconButton>

      <Separator orientation="vertical" size="1" />

      {/* ---- Home. ------------------------------------------------------- */}
      <IconButton variant="ghost" onClick={onHome}>
        <HomeIcon size={16} />
      </IconButton>

      <Separator orientation="vertical" size="1" />

      {/* ---- URL. -------------------------------------------------------- */}
      <Flex
        flexGrow="1"
        align="center"
        gap="2"
        px="3"
        py="1"
        className={toolbarPill()}
      >
        {state === "loading" ? <Spinner size="1" /> : <WorldIcon size={14} />}

        <Text size="1" color="gray" truncate>
          {url}
        </Text>
      </Flex>

      <Separator orientation="vertical" size="1" />

      {/* ---- Search. ----------------------------------------------------- */}
      <TextField.Root
        value={searchQuery}
        size="1"
        placeholder="Find in page"
        onChange={(event) => {
          onSearchQueryChanged(event.target.value);
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
        className={toolbarPill({ interactive: true })}
      >
        <TextField.Slot>
          <SearchIcon size={12} />
        </TextField.Slot>

        {searchQuery.trim() !== "" && (
          <>
            {searchResult && (
              <TextField.Slot side="right" pr="1">
                <Text size="1" color="gray">
                  {`${searchResult.activeMatch}/${searchResult.matches}`}
                </Text>
              </TextField.Slot>
            )}

            <TextField.Slot side="right" pr="2">
              <IconButton
                size="1"
                variant="ghost"
                color="gray"
                aria-label="Clear search"
                onClick={() => {
                  onSearchQueryChanged("");
                }}
              >
                <ClearIcon size={12} />
              </IconButton>
            </TextField.Slot>
          </>
        )}
      </TextField.Root>

      {searchQuery.trim() !== "" && (
        <>
          <IconButton variant="ghost" onClick={onSearchPrevious}>
            <FindPreviousIcon size={16} />
          </IconButton>

          <IconButton variant="ghost" onClick={onSearchNext}>
            <FindNextIcon size={16} />
          </IconButton>
        </>
      )}
    </Flex>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

const toolbarPill = cva(
  [
    "rounded-full border",
    "light:border-slate-300/90",
    "light:bg-white/82",
    "dark:border-slate-700/60",
    "dark:bg-slate-950/72",
  ],
  {
    variants: {
      interactive: {
        false: "",
        true: [
          "transition-colors",
          "light:hover:border-slate-400/90",
          "dark:hover:border-slate-600/70",
        ],
      },
    },
    defaultVariants: {
      interactive: false,
    },
  },
);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
