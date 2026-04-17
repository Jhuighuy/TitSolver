/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { IconPlus, IconTrash } from "@tabler/icons-react";
import { ReactNode, useState } from "react";

import { IconButton } from "~/renderer-common/components/button";
import { NumberInput, TextInput } from "~/renderer-common/components/input";
import { Flex } from "~/renderer-common/components/layout";
import { Select } from "~/renderer-common/components/select";
import { Switch } from "~/renderer-common/components/switch";
import { TreeTable } from "~/renderer-common/components/tree-table";
import type {
  ArraySpec,
  BoolSpec,
  EnumSpec,
  NumberSpec,
  RecordSpec,
  Spec,
  StringSpec,
  VariantSpec,
} from "~/shared/spec";
import { isTreeMap, normalizeTree, type Tree } from "~/shared/tree";
import { assert, iota } from "~/shared/utils";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// PropertyEditor
//

interface PropertyEditorProps {
  spec: Spec;
  tree: Tree;
  onTreeChanged: (tree: Tree) => void;
}

export function PropertyEditor({
  spec,
  tree,
  onTreeChanged,
}: Readonly<PropertyEditorProps>) {
  return (
    <TreeTable.Root>
      <TreeEditor
        label=""
        spec={spec}
        tree={tree}
        onTreeChanged={onTreeChanged}
      />
    </TreeTable.Root>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// TreeEditor
//

interface TreeEditorProps<S = Spec> {
  label: string;
  spec: S;
  tree: Tree;
  onTreeChanged: (tree: Tree) => void;
  onRemove?: () => void;
}

function TreeEditor({
  label,
  spec,
  tree,
  onTreeChanged,
  onRemove,
}: Readonly<TreeEditorProps>) {
  switch (spec.type) {
    case "bool":
      return (
        <BoolTreeEditor
          label={label}
          spec={spec}
          tree={tree}
          onTreeChanged={onTreeChanged}
          onRemove={onRemove}
        />
      );
    case "int":
    case "real":
      return (
        <NumberTreeEditor
          label={label}
          spec={spec}
          tree={tree}
          onTreeChanged={onTreeChanged}
          onRemove={onRemove}
        />
      );
    case "string":
      return (
        <StringTreeEditor
          label={label}
          spec={spec}
          tree={tree}
          onTreeChanged={onTreeChanged}
          onRemove={onRemove}
        />
      );
    case "enum":
      return (
        <EnumTreeEditor
          label={label}
          spec={spec}
          tree={tree}
          onTreeChanged={onTreeChanged}
          onRemove={onRemove}
        />
      );
    case "array":
      return (
        <ArrayTreeEditor
          label={label}
          spec={spec}
          tree={tree}
          onTreeChanged={onTreeChanged}
          onRemove={onRemove}
        />
      );
    case "record":
      return (
        <RecordTreeEditor
          label={label}
          spec={spec}
          tree={tree}
          onTreeChanged={onTreeChanged}
          onRemove={onRemove}
        />
      );
    case "variant":
      return (
        <VariantTreeEditor
          label={label}
          spec={spec}
          tree={tree}
          onTreeChanged={onTreeChanged}
          onRemove={onRemove}
        />
      );
  }
  assert(false);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// BoolTreeEditor
//

function BoolTreeEditor({
  label,
  tree,
  onTreeChanged,
  onRemove,
}: Readonly<TreeEditorProps<BoolSpec>>) {
  assert(typeof tree === "boolean");
  return (
    <TreeTable.Node
      label={label}
      value={
        <Removable onRemove={onRemove}>
          <Switch
            checked={tree}
            onCheckedChange={(checked) => {
              onTreeChanged(checked);
            }}
          />
        </Removable>
      }
    />
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// NumberTreeEditor
//

function NumberTreeEditor({
  label,
  spec,
  tree,
  onTreeChanged,
  onRemove,
}: Readonly<TreeEditorProps<NumberSpec>>) {
  assert(tree === null || typeof tree === "number");
  return (
    <TreeTable.Node
      label={label}
      color={tree === null ? "red" : undefined}
      value={
        <Removable onRemove={onRemove}>
          <NumberInput
            type={spec.type}
            value={tree}
            min={spec.min}
            max={spec.max}
            placeholder={tree === null ? "Required" : undefined}
            onValueChange={(v) => {
              onTreeChanged(v);
            }}
          />
        </Removable>
      }
    />
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// StringTreeEditor
//

function StringTreeEditor({
  label,
  tree,
  onTreeChanged,
  onRemove,
}: Readonly<TreeEditorProps<StringSpec>>) {
  assert(tree === null || typeof tree === "string");
  return (
    <TreeTable.Node
      label={label}
      color={tree === null ? "red" : undefined}
      value={
        <Removable onRemove={onRemove}>
          <TextInput
            value={tree ?? ""}
            placeholder={tree === null ? "Required" : undefined}
            onValueChange={(v) => {
              onTreeChanged(v.length > 0 ? v : null);
            }}
          />
        </Removable>
      }
    />
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// EnumTreeEditor
//

function EnumTreeEditor({
  label,
  spec,
  tree,
  onTreeChanged,
  onRemove,
}: Readonly<TreeEditorProps<EnumSpec>>) {
  assert(tree === null || typeof tree === "string");
  return (
    <TreeTable.Node
      label={label}
      color={tree === null ? "red" : undefined}
      value={
        <Removable onRemove={onRemove}>
          <Select.Root
            value={tree ?? undefined}
            onValueChange={(value) => {
              onTreeChanged(value);
            }}
          >
            <Select.Trigger />
            <Select.Content>
              {spec.options.map((opt) => (
                <Select.Item key={opt.id} value={opt.id}>
                  {opt.name}
                </Select.Item>
              ))}
            </Select.Content>
          </Select.Root>
        </Removable>
      }
    />
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// ArrayTreeEditor
//

function ArrayTreeEditor({
  label,
  spec,
  tree,
  onTreeChanged,
  onRemove,
}: Readonly<TreeEditorProps<ArraySpec>>) {
  assert(Array.isArray(tree));
  const array = tree;

  const [keys, setKeys] = useState(iota(array.length));
  const [nextKey, setNextKey] = useState(array.length);

  function handleAddItem() {
    const item = normalizeTree(spec.item, null).tree;
    const nextKeys = [...keys, nextKey];
    const nextArray = [...array, item];
    setKeys(nextKeys);
    setNextKey(nextKey + 1);
    onTreeChanged(nextArray);
  }

  function handleRemoveItem(index: number) {
    const nextKeys = keys.filter((_, i) => i !== index);
    const nextArray = array.filter((_, i) => i !== index);
    setKeys(nextKeys);
    onTreeChanged(nextArray);
  }

  function handleChangeItem(index: number, item: Tree) {
    const nextArray = array.map((current, i) => (i === index ? item : current));
    onTreeChanged(nextArray);
  }

  return (
    <TreeTable.Node
      label={label}
      value={
        <Removable onRemove={onRemove}>
          <IconButton aria-label="Add item" onClick={handleAddItem}>
            <IconPlus />
          </IconButton>
        </Removable>
      }
    >
      {tree.map((value, index) => (
        <TreeEditor
          key={keys[index]}
          label={`Item ${index + 1}`}
          spec={spec.item}
          tree={value}
          onTreeChanged={(item) => {
            handleChangeItem(index, item);
          }}
          onRemove={() => {
            handleRemoveItem(index);
          }}
        />
      ))}
    </TreeTable.Node>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// RecordTreeEditor
//

function RecordTreeEditor({
  label,
  spec,
  tree,
  onTreeChanged,
  onRemove,
}: Readonly<TreeEditorProps<RecordSpec>>) {
  assert(isTreeMap(tree));
  const map = tree;

  function handleFieldChange(id: string, field: Tree) {
    const nextMap = { ...map, [id]: field };
    onTreeChanged(nextMap);
  }

  const children = Object.entries(map).map(([id, field]) => {
    const fieldSpec = spec.fields.find((field) => field.id === id);
    assert(fieldSpec !== undefined);
    return (
      <TreeEditor
        key={id}
        label={fieldSpec.name}
        spec={fieldSpec.spec}
        tree={field}
        onTreeChanged={(field) => {
          handleFieldChange(id, field);
        }}
      />
    );
  });

  return label === "" ? (
    children
  ) : (
    <TreeTable.Node label={label} value={<Removable onRemove={onRemove} />}>
      {children}
    </TreeTable.Node>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// VariantTreeEditor
//

function VariantTreeEditor({
  label,
  spec,
  tree,
  onTreeChanged,
  onRemove,
}: Readonly<TreeEditorProps<VariantSpec>>) {
  assert(isTreeMap(tree));
  const map = tree;

  function handleActiveChange(_active: string) {
    const optionSpec = spec.options.find((option) => option.id === _active);
    assert(optionSpec !== undefined);
    const option = map[_active] ?? normalizeTree(optionSpec.spec, null).tree;
    const nextMap = { ...map, _active, [_active]: option };
    onTreeChanged(nextMap);
  }

  assert(typeof map._active === "string");
  const _active = map._active;

  function handleOptionChange(option: Tree) {
    const nextMap = { ...map, _active, [_active]: option };
    onTreeChanged(nextMap);
  }

  const activeSpec = spec.options.find((option) => option.id === _active);
  assert(activeSpec !== undefined);

  return (
    <TreeTable.Node
      label={label}
      value={
        <Removable onRemove={onRemove}>
          <Select.Root value={_active} onValueChange={handleActiveChange}>
            <Select.Trigger />
            <Select.Content>
              {spec.options.map(({ id, name }) => (
                <Select.Item key={id} value={id}>
                  {name}
                </Select.Item>
              ))}
            </Select.Content>
          </Select.Root>
        </Removable>
      }
    >
      <TreeEditor
        label=""
        spec={activeSpec.spec}
        tree={map[_active]}
        onTreeChanged={handleOptionChange}
      />
    </TreeTable.Node>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Removable
//

interface RemovableProps {
  onRemove?: () => void;
  children?: ReactNode;
}

function Removable({ onRemove, children }: Readonly<RemovableProps>) {
  return onRemove === undefined ? (
    children
  ) : (
    <Flex gap="1" align="center" justify="end">
      <Flex direction="column" flexGrow="1">
        {children}
      </Flex>
      <IconButton aria-label="Remove" onClick={onRemove}>
        <IconTrash />
      </IconButton>
    </Flex>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
