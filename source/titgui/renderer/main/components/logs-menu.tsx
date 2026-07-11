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
import { useAtomValue } from "jotai";
import { useEffect, useMemo, useRef, useState } from "react";

import { IconButton } from "~/renderer/common/components/button";
import {
  type MenuAction,
  useMenuAction,
} from "~/renderer/common/components/menu";
import { Text } from "~/renderer/common/components/text";
import { logger, type Message } from "~/renderer/common/logging";
import { downloadText } from "~/renderer/common/utils";
import { assert } from "~/shared/utils";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export function LogsMenu() {
  const messages = useAtomValue(logger.messagesAtom);

  // ---- Actions. -------------------------------------------------------------

  const saveAction = useMemo<MenuAction>(
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

  useMenuAction(saveAction);

  const clearAction = useMemo<MenuAction>(
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

  useMenuAction(clearAction);

  // ---- Layout. --------------------------------------------------------------

  const bottomRef = useRef<HTMLDivElement | null>(null);

  useEffect(() => {
    void messages;
    bottomRef.current?.scrollIntoView({ behavior: "smooth" });
  }, [messages]);

  return (
    <div className="size-full select-text">
      {messages.map(({ id, type, text }) => (
        <div key={id} className="p-1 even:bg-(--neutral-10)/5">
          <LogMessage type={type} text={text} />
        </div>
      ))}
      <div ref={bottomRef} />
    </div>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

type LogMessageProps = Pick<Message, "type" | "text">;

function LogMessage({ type, text }: Readonly<LogMessageProps>) {
  const [isExpanded, setIsExpanded] = useState(false);

  const colorClass = useMemo(() => {
    switch (type) {
      case "log":
        return "text-(--neutral-6)";
      case "warning":
        return "text-(--warning)";
      case "error":
        return "text-(--danger)";
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
      <div className="flex h-3 items-center gap-1">
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
      </div>
      {isExpanded && <pre>{text}</pre>}
    </Text>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
