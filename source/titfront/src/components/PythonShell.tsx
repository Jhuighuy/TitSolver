/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import {
  FC,
  Fragment,
  KeyboardEvent,
  MouseEvent,
  useEffect,
  useRef,
} from "react";
import {
  BiRadioCircle as CircleIcon,
  BiRadioCircleMarked as FilledCircleIcon,
} from "react-icons/bi";
import {
  FiCornerDownRight as EnterIcon,
  FiDollarSign as DollarIcon,
} from "react-icons/fi";

// import { useMenu } from "~/components/MainMenu";
import { PyError, usePython } from "~/components/Python";
import { usePythonShellStore } from "~/stores/PythonShellStore";
import { cn } from "~/utils";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/** @todo Implement a timeout for commands. */
export const PythonShell: FC = () => {
  const {
    commands,
    pushCommand,
    updateCommand,
    firstVisibleCommand,
    clearVisibleCommands,
    currentInput,
    setCurrentInput,
    historyIndex,
    historyUp,
    historyDown,
  } = usePythonShellStore();
  const inputRef = useRef<HTMLSpanElement>(null);
  const runCode = usePython();

  /*
  const { addAction } = useMenu();
  useEffect(() => {
    addAction({
      key: "python-shell",
      icon: <DollarIcon size={16} />,
      action: () => clearVisibleCommands(),
    });
  }, [addAction, clearVisibleCommands]);
  */

  const runCommand = (input: string) => {
    if (input.length === 0) {
      pushCommand({ input, status: "success" });
    } else {
      const commandIndex = commands.length;
      pushCommand({ input, status: "pending" });
      runCode(input, (result) => {
        updateCommand(commandIndex, {
          input,
          ...(result instanceof PyError
            ? { status: "failure", output: result.toString() }
            : { status: "success", output: JSON.stringify(result) }),
        });
      });
    }
  };

  const handleClick = (event: MouseEvent<HTMLElement>) => {
    if (!inputRef.current) return;
    event.preventDefault();

    // If anything is selected, don't focus the input. User will have to click
    // to unselect, and we'll focus the input then.
    const selection = window.getSelection();
    if (selection && selection.toString().length > 0) return;

    inputRef.current?.focus();
  };

  const handleKeyDown = (event: KeyboardEvent<HTMLElement>) => {
    const input = inputRef.current;
    if (!input) return;

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
  };

  const handleInput = (event: KeyboardEvent<HTMLElement>) => {
    setCurrentInput(event.currentTarget.textContent ?? "");
  };

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
  useEffect(
    () => inputRef.current?.scrollIntoView?.({ behavior: "smooth" }),
    [currentInput, commands]
  );

  const lineCn = "grid grid-cols-[20px_auto]";
  const textCn = "break-all whitespace-pre-wrap";
  const iconCn = "h-[1lh] flex items-center justify-center";
  return (
    <section
      data-testid="python-shell"
      className={cn(
        "size-full flex flex-col overflow-auto",
        "font-mono select-text bg-gray-900"
      )}
      aria-label="Python Shell"
      onClick={handleClick}
      onKeyDown={handleKeyDown}
    >
      {commands.slice(firstVisibleCommand).map((command, index) => (
        <Fragment key={index}>
          <div className={lineCn}>
            <div className={iconCn}>
              {command.status === "pending" ? (
                <CircleIcon />
              ) : (
                <FilledCircleIcon />
              )}
            </div>
            <pre className={textCn}>{command.input}</pre>
          </div>
          {command.output && (
            <div
              className={cn(
                lineCn,
                command.status === "failure" && "text-red-600"
              )}
            >
              <div className={iconCn}>
                <EnterIcon />
              </div>
              <pre className={textCn}>{command.output}</pre>
            </div>
          )}
        </Fragment>
      ))}
      <div className={lineCn}>
        <div className={iconCn}>
          <DollarIcon />
        </div>
        <span
          className={cn(textCn, "focus:outline-none")}
          aria-label="Command input"
          ref={inputRef}
          contentEditable="plaintext-only"
          spellCheck={false}
          onInput={handleInput}
        />
      </div>
    </section>
  );
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
