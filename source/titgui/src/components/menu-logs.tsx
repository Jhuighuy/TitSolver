/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { Box, Flex, IconButton, Text } from "@radix-ui/themes";
import { useEffect, useMemo, useRef, useState } from "react";
import {
  TbClearAll as ClearIcon,
  TbChevronLeft as CollapsedIcon,
  TbDownload as DownloadIcon,
  TbAlertCircle as ErrorIcon,
  TbChevronDown as ExpandedIcon,
  TbInfoSquare as InfoIcon,
  TbAlertTriangle as WarningIcon,
} from "react-icons/tb";

import { type MenuAction, useMenuAction } from "~/components/menu";
import { useSignalValue } from "~/hooks/use-signal";
import { logger, type Message } from "~/logging";
import { assert, downloadText } from "~/utils";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export function LogsMenu() {
  const messages = useSignalValue(logger.messages);

  // ---- Actions. -------------------------------------------------------------

  const stopAction = useMemo<MenuAction>(
    () => ({
      name: "Save Logs",
      icon: <DownloadIcon size={16} />,
      disabled: messages.length === 0,
      onClick: () =>
        downloadText(
          "logs.txt",
          messages
            .map(({ type, text }) => `${type.toUpperCase()}: ${text}`)
            .join("\n"),
        ),
    }),
    [messages],
  );

  useMenuAction(stopAction);

  const runAction = useMemo<MenuAction>(
    () => ({
      name: "Clear Logs",
      icon: <ClearIcon size={16} />,
      disabled: messages.length === 0,
      onClick: () => logger.clear(),
    }),
    [messages],
  );

  useMenuAction(runAction);

  // ---- Layout. --------------------------------------------------------------

  const bottomRef = useRef<HTMLDivElement | null>(null);

  useEffect(() => {
    if (!messages) return;
    bottomRef.current?.scrollIntoView({ behavior: "smooth" });
  }, [messages]);

  return (
    <Box width="100%" height="100%" className="select-text">
      {messages.map(({ id, type, text }) => (
        <Box key={id} p="1" className="even:bg-(--accent-3)">
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

  const color = useMemo(() => {
    switch (type) {
      case "log":
        return "gray";
      case "warning":
        return "orange";
      case "error":
        return "red";
    }
    assert(false);
  }, [type]);

  const icon = useMemo(() => {
    const iconSize = 16;
    switch (type) {
      case "log":
        return <InfoIcon size={iconSize} />;
      case "warning":
        return <WarningIcon size={iconSize} />;
      case "error":
        return <ErrorIcon size={iconSize} />;
    }
    assert(false);
  }, [type]);

  const [title, isMultiline] = useMemo(() => {
    const index = text.indexOf("\n");
    return index === -1 ? [text, false] : [text.slice(0, index), true];
  }, [text]);

  return (
    <Text size="1" color={color}>
      <Flex align="center" height="3" gap="1">
        {icon}
        {title}
        {isMultiline && (
          <IconButton
            size="1"
            variant="ghost"
            aria-label="Expand / collapse"
            onClick={() => setIsExpanded((prev) => !prev)}
          >
            {isExpanded ? (
              <ExpandedIcon size={10} />
            ) : (
              <CollapsedIcon size={10} />
            )}
          </IconButton>
        )}
      </Flex>
      {isExpanded && <pre>{text}</pre>}
    </Text>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
