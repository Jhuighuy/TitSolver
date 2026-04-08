/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import {
  IconAlertCircle,
  IconAlertTriangle,
  IconChevronDown,
  IconChevronLeft,
  IconClearAll,
  IconDownload,
  IconInfoSquare,
} from "@tabler/icons-react";
import { useEffect, useMemo, useRef, useState } from "react";

import { IconButton } from "~/renderer-common/components/button";
import { Box, Flex } from "~/renderer-common/components/layout";
import {
  type MenuAction,
  useMenuAction,
} from "~/renderer-common/components/menu";
import { Text } from "~/renderer-common/components/text";
import { useSignalValue } from "~/renderer-common/hooks/use-signal";
import { logger, type Message } from "~/renderer-common/logging";
import { downloadText } from "~/renderer-common/utils";
import { assert } from "~/shared/utils";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export function LogsMenu() {
  const messages = useSignalValue(logger.messages);

  // ---- Actions. -------------------------------------------------------------

  const stopAction = useMemo<MenuAction>(
    () => ({
      name: "Save Logs",
      icon: <IconDownload />,
      disabled: messages.length === 0,
      onClick: () => {
        downloadText(
          "logs.txt",
          messages
            .map(({ type, text }) => `${type.toUpperCase()}: ${text}`)
            .join("\n"),
        );
      },
    }),
    [messages],
  );

  useMenuAction(stopAction);

  const runAction = useMemo<MenuAction>(
    () => ({
      name: "Clear Logs",
      icon: <IconClearAll />,
      disabled: messages.length === 0,
      onClick: () => {
        logger.clear();
      },
    }),
    [messages],
  );

  useMenuAction(runAction);

  // ---- Layout. --------------------------------------------------------------

  const bottomRef = useRef<HTMLDivElement | null>(null);

  useEffect(() => {
    void messages;
    bottomRef.current?.scrollIntoView({ behavior: "smooth" });
  }, [messages]);

  return (
    <Box size="100%" className="select-text">
      {messages.map(({ id, type, text }) => (
        <Box key={id} p="1" className="even:bg-(--bg-6)">
          <LogMessage type={type} text={text} />
        </Box>
      ))}
      <div ref={bottomRef} />
    </Box>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

type LogMessageProps = Pick<Message, "type" | "text">;

function LogMessage({ type, text }: Readonly<LogMessageProps>) {
  const [isExpanded, setIsExpanded] = useState(false);

  const colorClass = useMemo(() => {
    switch (type) {
      case "log":
        return "text-(--fg-4)";
      case "warning":
        return "text-orange-600 dark:text-orange-400";
      case "error":
        return "text-red-600 dark:text-red-400";
    }
    assert(false);
  }, [type]);

  const icon = useMemo(() => {
    switch (type) {
      case "log":
        return <IconInfoSquare className="size-4 shrink-0" />;
      case "warning":
        return <IconAlertTriangle className="size-4 shrink-0" />;
      case "error":
        return <IconAlertCircle className="size-4 shrink-0" />;
    }
    assert(false);
  }, [type]);

  const [title, isMultiline] = useMemo(() => {
    const index = text.indexOf("\n");
    return index === -1 ? [text, false] : [text.slice(0, index), true];
  }, [text]);

  return (
    <Text className={colorClass}>
      <Flex align="center" gap="1" height="3">
        {icon}
        {title}
        {isMultiline && (
          <IconButton
            aria-label="Expand / collapse"
            onClick={() => {
              setIsExpanded((prev) => !prev);
            }}
          >
            {isExpanded ? <IconChevronDown /> : <IconChevronLeft />}
          </IconButton>
        )}
      </Flex>
      {isExpanded && <pre>{text}</pre>}
    </Text>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
