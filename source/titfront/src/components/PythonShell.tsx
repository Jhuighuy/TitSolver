/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { Code, Flex } from "@radix-ui/themes";
import type { ComponentProps } from "react";
import {
  Fragment,
  type KeyboardEvent,
  type MouseEvent,
  useEffect,
  useRef,
} from "react";
import type { IconType } from "react-icons";
import {
  BiRadioCircle as CircleIcon,
  BiRadioCircleMarked as FilledCircleIcon,
} from "react-icons/bi";
import {
  FiDollarSign as DollarIcon,
  FiCornerDownRight as EnterIcon,
} from "react-icons/fi";

import { useServer } from "~/components/Server";
import { usePythonShellStore } from "~/stores/PythonShellStore";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/** @todo Implement a timeout for commands. */
export function PythonShell() {
  // --- Command execution. ----------------------------------------------------

  const runCode = useServer();
  const {
    clearVisibleCommands,
    commands,
    currentInput,
    firstVisibleCommand,
    historyDown,
    historyIndex,
    historyUp,
    pushCommand,
    setCurrentInput,
    updateCommand,
  } = usePythonShellStore();

  const runCommand = (input: string) => {
    if (input.length === 0) {
      pushCommand({ input, status: "success" });
    } else {
      const commandIndex = commands.length;
      pushCommand({ input, status: "pending" });
      runCode(input, (result) => {
        const output = result as string | Error;
        updateCommand(commandIndex, {
          input,
          ...(output instanceof Error
            ? { status: "failure", output: output.toString() }
            : { status: "success", output: output.trimEnd() }),
        });
      });
    }
  };

  // --- Input handling. -------------------------------------------------------

  const inputRef = useRef<HTMLDivElement>(null);

  function handleClick(event: MouseEvent<HTMLElement>) {
    const input = inputRef.current;
    if (!input) return;
    event.preventDefault();

    // If anything is selected, don't focus the input. User will have to click
    // to unselect, and we'll focus the input then.
    const selection = window.getSelection();
    if (selection && selection.toString().length > 0) return;

    input.focus();
  }

  function handleKeyDown(event: KeyboardEvent<HTMLElement>) {
    // Cmd+K clears the console on macOS. Let's do the same.
    if (event.key === "k" && (event.ctrlKey || event.metaKey)) {
      event.preventDefault();
      clearVisibleCommands();
      return;
    }

    // Arrow keys for browsing history.
    if (event.key === "ArrowUp") {
      event.preventDefault();
      historyUp();
    } else if (event.key === "ArrowDown") {
      event.preventDefault();
      historyDown();
    }

    // Run the command.
    if (event.key === "Enter") {
      event.preventDefault();
      runCommand(currentInput.trim());
      setCurrentInput("");
    }
  }

  function handleInput(event: KeyboardEvent<HTMLElement>) {
    setCurrentInput(event.currentTarget.textContent ?? "");
  }

  // Keep the rendered input in sync with store.
  useEffect(() => {
    const input = inputRef.current;
    if (!input) return;

    // Update the input.
    input.textContent = currentInput;

    // If input is from history, move the cursor to the end.
    if (historyIndex !== null) {
      requestAnimationFrame(() => {
        const range = document.createRange();
        range.selectNodeContents(input);
        range.collapse(false);
        const selection = window.getSelection();
        if (selection) {
          selection.removeAllRanges();
          selection.addRange(range);
        }
      });
    }
  }, [currentInput, historyIndex]);

  // Scroll to bottom on input or command changes.
  // Note: `.scrollIntoView()` is not supported in testing environment.
  useEffect(() => inputRef.current?.scrollIntoView?.({ behavior: "smooth" }));

  // --- Layout. ---------------------------------------------------------------

  return (
    <Flex
      data-testid="python-shell"
      direction="column"
      width="100%"
      height="100%"
      overflow="auto"
      className="select-text hover:cursor-text"
      onClick={handleClick}
      onKeyDown={handleKeyDown}
    >
      {/* ---- Previous commands. ------------------------------------------ */}
      {commands
        .slice(firstVisibleCommand)
        .map(({ status, input, output }, index) => (
          // biome-ignore lint/suspicious/noArrayIndexKey: .
          <Fragment key={index}>
            <ShellLine
              icon={status === "pending" ? CircleIcon : FilledCircleIcon}
            >
              {input}
            </ShellLine>
            {output && (
              <ShellLine
                icon={EnterIcon}
                {...(status === "failure" && { color: "red" })}
              >
                {output}
              </ShellLine>
            )}
          </Fragment>
        ))}

      {/* ---- Current command input. -------------------------------------- */}
      <ShellLine
        ref={inputRef}
        icon={DollarIcon}
        variant="ghost"
        contentEditable="plaintext-only"
        spellCheck={false}
        onInput={handleInput}
      />
    </Flex>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

type ShellLineProps = ComponentProps<typeof Code> & {
  icon: IconType;
};

function ShellLine({ ref, icon: Icon, ...props }: ShellLineProps) {
  return (
    <Flex gap="1">
      <Flex align="center" justify="center" height="1lh">
        <Icon />
      </Flex>
      <Flex flexGrow="1" align="center">
        <Code
          ref={ref}
          variant="ghost"
          className="w-full break-all whitespace-pre-wrap"
          {...props}
        >
          {props.children}
        </Code>
      </Flex>
    </Flex>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
