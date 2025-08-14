/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { Box, Flex, Select, Switch, Text, TextField } from "@radix-ui/themes";
import {
  type ChangeEvent,
  type KeyboardEvent,
  useEffect,
  useState,
} from "react";
import {
  CiBoxList as EnumIcon,
  CiTextAlignLeft as TextIcon,
} from "react-icons/ci";
import { FaCaretDown as DownIcon, FaCaretUp as UpIcon } from "react-icons/fa";
import {
  MdDataArray as ArrayIcon,
  MdOutlineNumbers as NumbersIcon,
  MdOutlinePalette as PaletteIcon,
} from "react-icons/md";
import { PiTreeStructure as TreeIcon } from "react-icons/pi";
import { TbDecimal as DecimalIcon } from "react-icons/tb";

import { ColorBox } from "~/components/ColorBar";
import {
  type BoolProperty,
  type ColorMapProperty,
  type EnumProperty,
  type FloatProperty,
  getNestedItems,
  type HandleProperty,
  type IntProperty,
  type Property,
  type ScalarProperty,
  type StringProperty,
} from "~/components/Property";
import { assert } from "~/utils";
import { type ColorMapName, colorMaps } from "~/visual/ColorMap";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export interface PropertyEditorProps {
  property: ScalarProperty;
}

export function PropertyEditor({ property }: PropertyEditorProps) {
  switch (property.type) {
    case "bool":
      return <BoolPropertyEditor property={property} />;
    case "enum":
      return <EnumPropertyEditor property={property} />;
    case "colormap":
      return <ColorMapPropertyEditor property={property} />;
    case "string":
      return <StringPropertyEditor property={property} />;
    case "int":
      return <IntPropertyEditor property={property} />;
    case "float":
      return <FloatPropertyEditor property={property} />;
    case "handle":
      return <HandlePropertyEditor property={property} />;
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

interface BoolPropertyEditorProps {
  property: BoolProperty;
}

function BoolPropertyEditor({ property }: BoolPropertyEditorProps) {
  return (
    <Flex direction="row-reverse">
      <Switch disabled={property.disabled} defaultChecked={property.value} />
    </Flex>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

interface EnumPropertyEditorProps {
  property: EnumProperty;
}

function EnumPropertyEditor({ property }: EnumPropertyEditorProps) {
  return (
    <Select.Root
      disabled={property.disabled}
      value={property.value}
      onValueChange={property.setValue}
      size="1"
    >
      <Select.Trigger style={{ width: "100%" }}>
        <Flex align="center" gap="2">
          <EnumIcon size={10} />
          {property.value}
        </Flex>
      </Select.Trigger>
      <Select.Content position="popper">
        {property.options.map((option) => (
          <Select.Item key={option} value={option}>
            {option}
          </Select.Item>
        ))}
      </Select.Content>
    </Select.Root>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

interface ColorMapPropertyEditorProps {
  property: ColorMapProperty;
}

function ColorMapPropertyEditor({ property }: ColorMapPropertyEditorProps) {
  return (
    <Select.Root
      disabled={property.disabled}
      value={property.value}
      onValueChange={property.setValue}
      size="1"
    >
      <Select.Trigger style={{ width: "100%" }}>
        <Flex align="center" gap="2">
          <PaletteIcon size={10} />
          {colorMaps[property.value].label}
        </Flex>
      </Select.Trigger>
      <Select.Content position="popper">
        {Object.entries(colorMaps).map(([key, value]) => (
          <Select.Item key={key} value={key}>
            <Flex direction="row" align="center" gap="1">
              <ColorBox name={key as ColorMapName} width="80px" height="16px" />
              <Text>{value.label}</Text>
            </Flex>
          </Select.Item>
        ))}
      </Select.Content>
    </Select.Root>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

interface StringPropertyEditorProps {
  property: StringProperty;
}

function StringPropertyEditor({ property }: StringPropertyEditorProps) {
  function handleChange(event: ChangeEvent<HTMLInputElement>) {
    property.setValue(event.target.value);
  }

  return (
    <TextField.Root
      disabled={property.disabled}
      value={property.value}
      onChange={handleChange}
      size="1"
      placeholder="…"
    >
      <TextField.Slot>
        <TextIcon size={16} />
      </TextField.Slot>
    </TextField.Root>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

interface IntPropertyEditorProps {
  property: IntProperty;
}

function IntPropertyEditor({ property }: IntPropertyEditorProps) {
  const [valid, setValid] = useState(true);
  const [stringValue, setStringValue] = useState("");

  useEffect(() => {
    setStringValue(`${property.value}`);
  }, [property.value]);

  // --- Up / Down buttons. ----------------------------------------------------

  function handleIncrement() {
    if (!valid || property.value === undefined) return;
    const newValue = property.value + 1;
    if (property.max === undefined || newValue <= property.max) {
      setStringValue(`${newValue}`);
      property.setValue(newValue);
    }
  }

  function handleDecrement() {
    if (!valid || property.value === undefined) return;
    const newValue = property.value - 1;
    if (property.min === undefined || newValue >= property.min) {
      setStringValue(`${newValue}`);
      property.setValue(newValue);
    }
  }

  // --- Input handling. -------------------------------------------------------

  function handleChange(event: ChangeEvent<HTMLInputElement>) {
    const inputValue = event.target.value;
    setStringValue(inputValue);

    // Check if the input is a valid integer.
    const valid = /^-?\d+$/.test(inputValue);
    setValid(valid);
    if (!valid) return;

    // Check if the input is within the min/max bounds.
    const intValue = Number.parseInt(inputValue, 10);
    if (property.min !== undefined && intValue < property.min) {
      setValid(false);
      return;
    }
    if (property.max !== undefined && intValue > property.max) {
      setValid(false);
      return;
    }

    // Update the actual value.
    property.setValue(intValue);
  }

  // --- Layout. ---------------------------------------------------------------

  return (
    <TextField.Root
      disabled={property.disabled}
      value={stringValue}
      onChange={handleChange}
      color={valid ? undefined : "red"}
      size="1"
      placeholder="…"
    >
      <TextField.Slot>
        <NumbersIcon size={16} />
      </TextField.Slot>
      {!property.disabled && (
        <TextField.Slot>
          <Flex direction="column" overflow="hidden">
            <Box
              asChild
              onClick={handleIncrement}
              className="hover:text-gray-100 cursor-auto"
            >
              <UpIcon width="20" height="6" />
            </Box>
            <Box
              asChild
              onClick={handleDecrement}
              className="hover:text-gray-100 cursor-auto"
            >
              <DownIcon width="20" height="6" />
            </Box>
          </Flex>
        </TextField.Slot>
      )}
    </TextField.Root>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

interface FloatPropertyEditorProps {
  property: FloatProperty;
}

function FloatPropertyEditor({ property }: FloatPropertyEditorProps) {
  const [valid, setValid] = useState(true);
  const [stringValue, setStringValue] = useState("");

  useEffect(() => {
    setStringValue(`${property.value}`);
  }, [property.value]);

  // --- Up / Down buttons. ----------------------------------------------------

  const delta =
    property.min !== undefined && property.max !== undefined
      ? (property.max - property.min) / 100
      : 1;

  function handleIncrement() {
    if (!valid || property.value === undefined) return;
    const newValue = property.value + delta;
    if (property.max === undefined || newValue <= property.max) {
      setStringValue(`${newValue}`);
      property.setValue(newValue);
    }
  }

  function handleDecrement() {
    if (!valid || property.value === undefined) return;
    const newValue = property.value - delta;
    if (property.min === undefined || newValue >= property.min) {
      setStringValue(`${newValue}`);
      property.setValue(newValue);
    }
  }

  // --- Input handling. -------------------------------------------------------

  function handleChange(event: ChangeEvent<HTMLInputElement>) {
    const inputValue = event.target.value;
    setStringValue(inputValue);

    // Check if the input is a valid float.
    const valid = /^[+-]?\d+(\.\d+)?$/.test(inputValue);
    setValid(valid);
    if (!valid) return;

    // Check if the input is within the min/max bounds.
    const floatValue = Number.parseFloat(inputValue);
    if (property.min !== undefined && floatValue < property.min) {
      setValid(false);
      return;
    }
    if (property.max !== undefined && floatValue > property.max) {
      setValid(false);
      return;
    }

    // Update the actual value.
    property.setValue?.(floatValue);
  }

  function handleKeyUp(event: KeyboardEvent<HTMLInputElement>) {
    switch (event.key) {
      case "ArrowUp":
        event.preventDefault();
        handleIncrement();
        break;
      case "ArrowDown":
        event.preventDefault();
        handleDecrement();
        break;
    }
  }

  function handleBlur() {
    if (!valid) setStringValue(`${property.value}`);
  }

  // --- Layout. ---------------------------------------------------------------

  return (
    <TextField.Root
      disabled={property.disabled}
      value={stringValue}
      onChange={handleChange}
      onBlur={handleBlur}
      onKeyUp={handleKeyUp}
      color={valid ? undefined : "red"}
      size="1"
      placeholder="…"
    >
      <TextField.Slot>
        <DecimalIcon size={16} />
      </TextField.Slot>
      {!property.disabled && (
        <TextField.Slot>
          <Flex direction="column" overflow="hidden">
            <Box
              asChild
              onClick={handleIncrement}
              className="hover:text-gray-100 cursor-auto"
            >
              <UpIcon width="20" height="6" />
            </Box>
            <Box
              asChild
              onClick={handleDecrement}
              className="hover:text-gray-100 cursor-auto"
            >
              <DownIcon width="20" height="6" />
            </Box>
          </Flex>
        </TextField.Slot>
      )}
    </TextField.Root>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

interface HandlePropertyEditorProps {
  property: HandleProperty;
}

function HandlePropertyEditor({ property }: HandlePropertyEditorProps) {
  function stringify(p: Property): string {
    switch (p.type) {
      case "bool":
      case "int":
      case "float":
        return `${p.value}`;
      case "enum":
      case "string":
        return `"${p.value}"`;
      case "record":
        return `{${getNestedItems(p)
          .map((x) => `"${x.name}": ${stringify(x)}`)
          .join(", ")}}`;
      case "array":
        return `[${p.children.map((x) => stringify(x.props)).join(", ")}]`;
    }
    assert(false);
  }

  return (
    <TextField.Root value={stringify(property.target)} disabled={true} size="1">
      <TextField.Slot>
        {property.target.type === "record" ? (
          <TreeIcon size={16} />
        ) : (
          <ArrayIcon size={16} />
        )}
      </TextField.Slot>
    </TextField.Root>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
