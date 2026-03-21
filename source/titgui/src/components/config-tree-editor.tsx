/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import {
  Box,
  Button,
  DropdownMenu,
  Flex,
  IconButton,
  Select,
  Switch,
  Text,
  TextField,
} from "@radix-ui/themes";
import { type ReactNode, useEffect, useState } from "react";
import {
  TbChevronDown,
  TbChevronRight,
  TbClipboardText,
  TbCopy,
  TbDots,
  TbPlus,
  TbReload,
  TbTrash,
} from "react-icons/tb";

import { NumberEditor } from "~/components/number-editor";
import { copyJsonToClipboard, readJsonFromClipboard } from "~/solver-config-io";
import {
  createDefaultValue,
  hasDefaultValue,
  isPropertyTreeCompatible,
  type PropertyRecord,
  type PropertySpec,
  type PropertyTree,
  type RootSpec,
} from "~/solver-config";
import {
  getValueAtPath,
  pathKey,
  removeValueAtPath,
  setValueAtPath,
  type Path,
} from "~/solver-config-tree";
import { type SetStateAction } from "~/utils";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

const TREE_INDENT_PX = 12;
const TREE_TOGGLE_SLOT_PX = 12;
const ROW_ICON_BUTTON_PX = 12;

type ConfigTreeEditorProps = {
  spec: RootSpec;
  tree: PropertyRecord;
  onTreeChange: (next: SetStateAction<PropertyRecord>) => void;
};

export function ConfigTreeEditor({
  spec,
  tree,
  onTreeChange,
}: Readonly<ConfigTreeEditorProps>) {
  const [expanded, setExpanded] = useState(() => new Set<string>([""]));

  function toggleExpanded(path: Path) {
    const key = pathKey(path);
    setExpanded((prev) => {
      const next = new Set(prev);
      if (next.has(key)) next.delete(key);
      else next.add(key);
      return next;
    });
  }

  return (
    <Box className="overflow-hidden rounded-md border border-slate-700/60">
      <table className="w-full table-auto border-collapse">
        <thead>
          <tr className="border-b border-slate-700/60">
            <th className="border-r border-slate-700/60 px-4 py-2 text-left text-sm font-semibold whitespace-nowrap">
              Property
            </th>
            <th className="w-full px-4 py-2 text-left text-sm font-semibold">
              Value
            </th>
            <th className="w-10 pl-1 pr-2 py-2" />
          </tr>
        </thead>
        <tbody>
          {spec.fields.map((field) => (
            <ConfigRow
              key={field.id}
              depth={0}
              fieldName={field.name}
              path={[field.id]}
              spec={field.spec}
              value={tree[field.id]}
              expanded={expanded}
              onToggleExpanded={toggleExpanded}
              onTreeChange={onTreeChange}
            />
          ))}
        </tbody>
      </table>
    </Box>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

type ConfigRowProps = {
  depth: number;
  fieldName: string;
  path: Path;
  spec: PropertySpec;
  value: PropertyTree;
  expanded: Set<string>;
  onToggleExpanded: (path: Path) => void;
  onTreeChange: (next: SetStateAction<PropertyRecord>) => void;
};

function ConfigRow({
  depth,
  fieldName,
  path,
  spec,
  value,
  expanded,
  onToggleExpanded,
  onTreeChange,
}: Readonly<ConfigRowProps>) {
  const expandable =
    spec.type === "record" || spec.type === "array" || spec.type === "variant";
  const isOpen = expanded.has(pathKey(path));
  const resolvedValue = isPropertyTreeCompatible(spec, value)
    ? value
    : createDefaultValue(spec);
  const resolvedRecordValue =
    spec.type === "record" || spec.type === "variant"
      ? (resolvedValue as PropertyRecord)
      : null;
  const resolvedArrayValue =
    spec.type === "array" ? (resolvedValue as PropertyTree[]) : null;

  async function handleCopy() {
    await copyJsonToClipboard(resolvedValue);
  }

  async function handlePaste() {
    const next = await readJsonFromClipboard();
    if (!isPropertyTreeCompatible(spec, next)) return;
    onTreeChange((prev) => setValueAtPath(prev, path, next));
  }

  function handleReset() {
    onTreeChange((prev) =>
      setValueAtPath(prev, path, createDefaultValue(spec)),
    );
  }

  function handleRemove() {
    onTreeChange((prev) => removeValueAtPath(prev, path));
  }

  return (
    <>
      <tr className="border-b border-slate-700/60 align-middle even:bg-(--accent-3)">
        <th className="border-r border-slate-700/60 pl-2 pr-1 py-1.5 text-left font-normal whitespace-nowrap">
          <Flex align="center" gap="2" height="3" className="whitespace-nowrap">
            {renderTreeSlots(
              depth,
              expandable ? (
                <IconButton
                  size="1"
                  variant="ghost"
                  style={{
                    width: `${ROW_ICON_BUTTON_PX}px`,
                    height: `${ROW_ICON_BUTTON_PX}px`,
                  }}
                  onClick={() => onToggleExpanded(path)}
                >
                  {isOpen ? (
                    <TbChevronDown size={14} />
                  ) : (
                    <TbChevronRight size={14} />
                  )}
                </IconButton>
              ) : undefined,
            )}
            <Text size="1" className="whitespace-nowrap">
              {fieldName}
            </Text>
          </Flex>
        </th>

        <td className="w-full min-w-0 pl-1 pr-1 py-1.5">
          <Box width="100%">
            {renderValueEditor(spec, resolvedValue, path, onTreeChange)}
          </Box>
        </td>

        <td className="w-10 pl-1 pr-2 py-1.5">
          <Flex align="center" justify="center">
            <DropdownMenu.Root>
              <DropdownMenu.Trigger>
                <IconButton
                  size="1"
                  variant="ghost"
                  style={{
                    width: `${ROW_ICON_BUTTON_PX}px`,
                    height: `${ROW_ICON_BUTTON_PX}px`,
                  }}
                >
                  <TbDots size={14} />
                </IconButton>
              </DropdownMenu.Trigger>
              <DropdownMenu.Content>
                <DropdownMenu.Item onClick={() => void handleCopy()}>
                  <TbCopy size={14} />
                  Copy JSON
                </DropdownMenu.Item>
                <DropdownMenu.Item onClick={() => void handlePaste()}>
                  <TbClipboardText size={14} />
                  Paste JSON
                </DropdownMenu.Item>
                {hasDefaultValue(spec) && (
                  <DropdownMenu.Item onClick={handleReset}>
                    <TbReload size={14} />
                    Reset Default
                  </DropdownMenu.Item>
                )}
                {typeof path.at(-1) === "number" && (
                  <DropdownMenu.Item color="red" onClick={handleRemove}>
                    <TbTrash size={14} />
                    Remove
                  </DropdownMenu.Item>
                )}
              </DropdownMenu.Content>
            </DropdownMenu.Root>
          </Flex>
        </td>
      </tr>

      {expandable && isOpen && (
        <>
          {spec.type === "record" &&
            spec.fields.map((field) => (
              <ConfigRow
                key={field.id}
                depth={depth + 1}
                fieldName={field.name}
                path={[...path, field.id]}
                spec={field.spec}
                value={(resolvedRecordValue as PropertyRecord)[field.id]}
                expanded={expanded}
                onToggleExpanded={onToggleExpanded}
                onTreeChange={onTreeChange}
              />
            ))}

          {spec.type === "array" &&
            (resolvedArrayValue as PropertyTree[]).map((item, index) => (
              <ConfigRow
                key={index}
                depth={depth + 1}
                fieldName={`[${index}]`}
                path={[...path, index]}
                spec={spec.items}
                value={item}
                expanded={expanded}
                onToggleExpanded={onToggleExpanded}
                onTreeChange={onTreeChange}
              />
            ))}

          {spec.type === "variant" &&
            (() => {
              const active = getVariantActive(resolvedValue);
              const option = spec.options[active];
              if (!option) return null;
              return (
                <ConfigRow
                  depth={depth + 1}
                  fieldName={option.name}
                  path={[...path, active]}
                  spec={option.spec}
                  value={(resolvedRecordValue as PropertyRecord)[active]}
                  expanded={expanded}
                  onToggleExpanded={onToggleExpanded}
                  onTreeChange={onTreeChange}
                />
              );
            })()}

          {spec.type === "array" && (
            <tr className="border-b border-slate-700/60 align-middle even:bg-(--accent-3)">
              <td className="border-r border-slate-700/60 px-2 py-1.5">
                <Flex align="center" gap="2" height="3">
                  {renderTreeSlots(depth + 1)}
                  <Button
                    size="1"
                    variant="soft"
                    onClick={() =>
                      onTreeChange((prev) => {
                        const currentItems = getValueAtPath(prev, path);
                        return setValueAtPath(prev, path, [
                          ...(Array.isArray(currentItems) ? currentItems : []),
                          createDefaultValue(spec.items),
                        ]);
                      })
                    }
                  >
                    <TbPlus size={14} />
                    Add Item
                  </Button>
                </Flex>
              </td>
              <td className="pl-4 pr-1 py-1.5" />
              <td className="pl-1 pr-2 py-1.5" />
            </tr>
          )}
        </>
      )}
    </>
  );
}

function renderValueEditor(
  spec: PropertySpec,
  value: PropertyTree,
  path: Path,
  onTreeChange: (next: SetStateAction<PropertyRecord>) => void,
) {
  switch (spec.type) {
    case "bool":
      return (
        <Switch
          checked={value as boolean}
          onCheckedChange={(checked) =>
            onTreeChange((prev) => setValueAtPath(prev, path, checked))
          }
        />
      );
    case "int":
      return (
        <NumberEditor
          className="w-full"
          size="1"
          type="int"
          min={spec.min}
          max={spec.max}
          value={value as number}
          onValueChange={(next) =>
            onTreeChange((prev) => setValueAtPath(prev, path, next))
          }
        />
      );
    case "real":
      return (
        <NumberEditor
          className="w-full"
          size="1"
          min={spec.min}
          max={spec.max}
          value={value as number}
          onValueChange={(next) =>
            onTreeChange((prev) => setValueAtPath(prev, path, next))
          }
        />
      );
    case "string":
      return (
        <CommittedTextField
          value={value as string}
          onCommit={(next) =>
            onTreeChange((prev) => setValueAtPath(prev, path, next))
          }
        />
      );
    case "enum":
      return (
        <Box width="100%">
          <Select.Root
            size="1"
            value={value as string}
            onValueChange={(next) =>
              onTreeChange((prev) => setValueAtPath(prev, path, next))
            }
          >
            <Select.Trigger style={{ width: "100%" }} />
            <Select.Content>
              {spec.options.map((option) => (
                <Select.Item key={option.id} value={option.id}>
                  {option.name}
                </Select.Item>
              ))}
            </Select.Content>
          </Select.Root>
        </Box>
      );
    case "record":
      return (
        <Text size="1" color="gray">
          {spec.fields.length} field{spec.fields.length === 1 ? "" : "s"}
        </Text>
      );
    case "array":
      return (
        <Text size="1" color="gray">
          {(value as PropertyTree[]).length} item
          {(value as PropertyTree[]).length === 1 ? "" : "s"}
        </Text>
      );
    case "variant": {
      const active = getVariantActive(value);
      return (
        <Box width="100%">
          <Select.Root
            size="1"
            value={active}
            onValueChange={(next) =>
              onTreeChange((prev) => {
                const option = spec.options[next];
                return setValueAtPath(prev, path, {
                  _active: next,
                  [next]: createDefaultValue(option.spec),
                });
              })
            }
          >
            <Select.Trigger style={{ width: "100%" }} />
            <Select.Content>
              {Object.entries(spec.options).map(([id, option]) => (
                <Select.Item key={id} value={id}>
                  {option.name}
                </Select.Item>
              ))}
            </Select.Content>
          </Select.Root>
        </Box>
      );
    }
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

type CommittedTextFieldProps = {
  value: string;
  onCommit: (value: string) => void;
};

function CommittedTextField({
  value,
  onCommit,
}: Readonly<CommittedTextFieldProps>) {
  const [text, setText] = useState(value);

  useEffect(() => {
    setText(value);
  }, [value]);

  function commit() {
    onCommit(text);
  }

  return (
    <TextField.Root
      size="1"
      value={text}
      onChange={(event) => setText(event.target.value)}
      onBlur={commit}
      onKeyDown={(event) => {
        if (event.key === "Escape") {
          event.preventDefault();
          setText(value);
        }
        if (event.key === "Enter") {
          event.preventDefault();
          commit();
        }
      }}
    />
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

function renderTreeSlots(depth: number, toggle?: ReactNode) {
  return (
    <>
      {Array.from({ length: depth }).map((_, index) => (
        <Flex
          key={index}
          align="center"
          justify="center"
          width={`${TREE_INDENT_PX}px`}
          minWidth={`${TREE_INDENT_PX}px`}
          height="100%"
        >
          <Box
            width="1px"
            height="1.5em"
            style={{
              backgroundColor: "var(--accent-8)",
              opacity: 0.8,
            }}
          />
        </Flex>
      ))}
      <Flex
        align="center"
        justify="center"
        width={`${TREE_TOGGLE_SLOT_PX}px`}
        minWidth={`${TREE_TOGGLE_SLOT_PX}px`}
      >
        {toggle ?? (
          <Box
            width="1px"
            height="1.5em"
            style={{
              backgroundColor: "var(--accent-8)",
              opacity: 0.8,
            }}
          />
        )}
      </Flex>
    </>
  );
}

function getVariantActive(value: PropertyTree): string {
  return (value as PropertyRecord)._active as string;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
