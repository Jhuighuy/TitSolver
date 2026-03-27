/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { TextField } from "@radix-ui/themes";
import { useEffect, useState } from "react";

import { assert } from "~/shared/utils";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

interface NumberEditorProps {
  className?: string;
  disabled?: boolean;
  size?: TextField.RootProps["size"];
  color?: TextField.RootProps["color"];
  label?: string;
  type?: "int" | "float";
  min?: number;
  max?: number;
  value: number;
  onValueChange: (value: number) => void;
}

export function NumberEditor({
  className,
  disabled = false,
  size,
  color,
  label,
  type = "float",
  min = Number.NEGATIVE_INFINITY,
  max = Number.POSITIVE_INFINITY,
  value,
  onValueChange,
}: Readonly<NumberEditorProps>) {
  assert(type === "float" || Number.isInteger(value));
  assert(min <= value && value <= max);

  // ---- State. --------------------------------------------------------------

  const [text, setText] = useState(value.toString());

  useEffect(() => {
    setText(value.toString());
  }, [value]);

  // ---- Number. --------------------------------------------------------------

  const parsed = Number(text);
  const isValid =
    Number.isFinite(parsed) &&
    (type === "float" || Number.isInteger(parsed)) &&
    parsed >= min &&
    parsed <= max;

  // ---- Events. --------------------------------------------------------------

  function handleBlur() {
    if (!isValid) return;
    setText(parsed.toString());
    onValueChange(parsed);
  }

  function handleKeyDown(event: React.KeyboardEvent) {
    if (event.key === "Escape") {
      event.preventDefault();
      setText(value.toString());
    }
    if (event.key === "Enter") {
      event.preventDefault();
      handleBlur();
    }
  }

  // ---- Layout. --------------------------------------------------------------

  return (
    <TextField.Root
      className={className}
      disabled={disabled}
      size={size}
      value={text}
      color={isValid ? color : "red"}
      onChange={(event) => {
        setText(event.target.value);
      }}
      onBlur={handleBlur}
      onKeyDown={handleKeyDown}
    >
      {label && <TextField.Slot side="left">{label}</TextField.Slot>}
    </TextField.Root>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
