/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { Input as BaseInput } from "@base-ui/react/input";
import { NumberField as BaseNumberField } from "@base-ui/react/number-field";
import { IconChevronDown, IconChevronUp } from "@tabler/icons-react";
import { cva, type VariantProps } from "class-variance-authority";
import type {
  ChangeEvent,
  ComponentProps,
  FocusEventHandler,
  KeyboardEventHandler,
  ReactNode,
} from "react";

import { cn } from "~/renderer-common/components/utils";
import { assert } from "~/shared/utils";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

const inputRootVariants = cva(
  "inline-flex items-center overflow-hidden border bg-(--bg-3) transition-colors",
  {
    variants: {
      invalid: {
        false: "border-(--chrome-1) hover:border-(--chrome-2)",
        true: "border-red-400 dark:border-red-600",
      },
      size: {
        "1": "h-6 text-(length:--text-1) leading-(--leading-1)",
        "2": "h-7 text-(length:--text-2) leading-(--leading-2)",
        "3": "h-8 text-(length:--text-3) leading-(--leading-3)",
      },
      radius: {
        none: "rounded-none",
        small: "rounded-sm",
        medium: "rounded",
        large: "rounded-lg",
        full: "rounded-full",
      },
    },
    defaultVariants: {
      invalid: false,
      size: "1",
      radius: "medium",
    },
  },
);

const inputElementClasses = cn(
  "h-full min-w-0 flex-1 bg-transparent px-2 text-(--fg-2) outline-none placeholder:text-(--fg-5) disabled:cursor-not-allowed",
);

const inputSlotClasses = cn(
  "flex shrink-0 items-center pl-2 text-(--fg-4) [&_svg]:size-[1.5em] [&_svg]:shrink-0",
);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

interface TextInputProps
  extends
    Omit<ComponentProps<"div">, "onBlur" | "onChange" | "slot">,
    VariantProps<typeof inputRootVariants> {
  disabled?: boolean;
  slot?: ReactNode;
  value?: string;
  placeholder?: string;
  onValueChange: (value: string, event: ChangeEvent<HTMLInputElement>) => void;
  onBlur?: FocusEventHandler<HTMLInputElement>;
  onKeyDown?: KeyboardEventHandler<HTMLInputElement>;
}

export function TextInput({
  disabled,
  size,
  radius,
  slot,
  value,
  placeholder,
  onValueChange,
  onBlur,
  onKeyDown,
  className,
  children,
  ...props
}: Readonly<TextInputProps>) {
  return (
    <div
      {...props}
      className={cn(
        inputRootVariants({ size, radius }),
        disabled && "cursor-not-allowed opacity-40",
        className,
      )}
    >
      {slot && <div className={inputSlotClasses}>{slot}</div>}
      <BaseInput
        disabled={disabled}
        value={value}
        onPointerDownCapture={(event) => {
          event.stopPropagation();
        }}
        onClickCapture={(event) => {
          event.stopPropagation();
        }}
        onKeyDownCapture={(event) => {
          event.stopPropagation();
        }}
        onChange={(event) => {
          onValueChange(event.target.value, event);
        }}
        onBlur={onBlur}
        onKeyDown={onKeyDown}
        placeholder={placeholder}
        className={inputElementClasses}
      />
      {children !== undefined && (
        <div className={inputSlotClasses}>{children}</div>
      )}
    </div>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

const stepperButtonClasses = cn(
  "flex min-h-0 w-5 flex-1 cursor-pointer items-center justify-center text-(--fg-4) transition-colors select-none hover:bg-(--bg-4) hover:text-(--fg-1) disabled:cursor-not-allowed disabled:opacity-40 [&_svg]:size-[1.5em] [&_svg]:shrink-0",
);

interface NumberInputProps extends VariantProps<typeof inputRootVariants> {
  className?: string;
  disabled?: boolean;
  slot?: ReactNode;
  placeholder?: string;
  type?: "int" | "real";
  min?: number;
  max?: number;
  step?: number;
  value: number | null;
  onValueChange: (value: number | null) => void;
  onBlur?: FocusEventHandler<HTMLInputElement>;
}

export function NumberInput({
  className,
  disabled = false,
  size,
  radius,
  slot,
  placeholder,
  type = "real",
  min = Number.NEGATIVE_INFINITY,
  max = Number.POSITIVE_INFINITY,
  step,
  value,
  onValueChange,
  onBlur,
}: Readonly<NumberInputProps>) {
  assert(
    value === null ||
      ((type === "real" || Number.isInteger(value)) &&
        min <= value &&
        value <= max),
  );

  if (step === undefined) {
    if (type === "int") {
      step = 1;
    } else if (Number.isFinite(min) && Number.isFinite(max)) {
      step = (max - min) / 10;
    } else {
      step = 0.1;
    }
  }

  return (
    <BaseNumberField.Root
      disabled={disabled}
      value={value}
      locale="en-US"
      min={min}
      max={max}
      step={step}
      smallStep={step}
      largeStep={step * 10}
      onValueChange={(next) => {
        if (type === "int" && !Number.isInteger(next)) return;
        onValueChange(next);
      }}
      className={cn("min-w-0", className)}
    >
      <BaseNumberField.Group
        className={({ valid }) =>
          cn(
            "group flex w-full",
            inputRootVariants({ size, radius, invalid: valid === false }),
            disabled && "cursor-not-allowed opacity-40",
          )
        }
      >
        {slot && <div className={inputSlotClasses}>{slot}</div>}

        <BaseNumberField.Input
          placeholder={placeholder}
          inputMode={type === "int" ? "numeric" : "decimal"}
          onPointerDownCapture={(event) => {
            event.stopPropagation();
          }}
          onClickCapture={(event) => {
            event.stopPropagation();
          }}
          onKeyDownCapture={(event) => {
            event.stopPropagation();
          }}
          onBlur={onBlur}
          className={cn(inputElementClasses, "font-(--font-mono)")}
        />

        <div className="flex h-full shrink-0 flex-col border-l border-(--chrome-1) group-data-invalid:border-red-400 group-data-invalid:dark:border-red-600">
          <BaseNumberField.Increment className={stepperButtonClasses}>
            <IconChevronUp />
          </BaseNumberField.Increment>
          <BaseNumberField.Decrement className={stepperButtonClasses}>
            <IconChevronDown />
          </BaseNumberField.Decrement>
        </div>
      </BaseNumberField.Group>
    </BaseNumberField.Root>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
