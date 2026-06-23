/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { IconPlus, IconTrash } from "@tabler/icons-react";
import { createContext, type ReactNode, useContext, useMemo } from "react";

import { IconButton } from "~/renderer/common/components/button";
import { NumberInput, TextInput } from "~/renderer/common/components/input";
import { Flex } from "~/renderer/common/components/layout";
import { Select } from "~/renderer/common/components/select";
import { Switch } from "~/renderer/common/components/switch";
import { TreeTable } from "~/renderer/common/components/tree-table";
import {
  isTreeMap,
  type ArraySpec,
  type BoolSpec,
  type EnumSpec,
  type NamespaceTable,
  type NumberSpec,
  type RefSpec,
  type RecordSpec,
  type Spec,
  type StringSpec,
  type SymbolSpec,
  type Tree,
  type VariantSpec,
} from "~/shared/properties";
import { assert } from "~/shared/utils";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// PropertyEditor
//

interface PropertyEditorProps {
  spec: Spec;
  tree: Tree;
  namespaces: NamespaceTable;
  onTreeChanged: (tree: Tree) => void;
}

export function PropertyEditor({
  spec,
  tree,
  namespaces,
  onTreeChanged,
}: Readonly<PropertyEditorProps>) {
  const context = useMemo(() => ({ namespaces }), [namespaces]);
  return (
    <PropertyEditorContext.Provider value={context}>
      <TreeTable.Root>
        <TreeEditor
          label=""
          spec={spec}
          tree={tree}
          onTreeChanged={onTreeChanged}
        />
      </TreeTable.Root>
    </PropertyEditorContext.Provider>
  );
}

interface PropertyEditorContext {
  namespaces: NamespaceTable;
}

const PropertyEditorContext = createContext<PropertyEditorContext | null>(null);

function usePropertyEditorContext() {
  const context = useContext(PropertyEditorContext);
  assert(context !== null);
  return context;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// TreeEditor
//

interface TreeEditorProps<S extends Spec = Spec> {
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
    case "symbol":
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
    case "ref":
      return (
        <RefTreeEditor
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
            onValueChange={(value) => {
              onTreeChanged(value);
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
}: Readonly<TreeEditorProps<StringSpec | SymbolSpec>>) {
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
            onValueChange={(value) => {
              onTreeChanged(value.length > 0 ? value : null);
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
            value={tree}
            onValueChange={(value) => {
              onTreeChanged(value);
            }}
          >
            <Select.Trigger />
            <Select.Content>
              {spec.options.map((optionSpec) => (
                <Select.Item key={optionSpec.id} value={optionSpec.id}>
                  {optionSpec.name}
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

  function handleAddItem() {
    onTreeChanged([...array, null]);
  }

  function handleRemoveItem(index: number) {
    onTreeChanged(array.filter((_, i) => i !== index));
  }

  function handleChangeItem(index: number, item: Tree) {
    onTreeChanged(array.map((current, i) => (i === index ? item : current)));
  }

  const itemSpec = spec.item;
  const itemKeyCounts = new Map<string, number>();
  const keyedItems = tree.map((item) => {
    const baseKey = JSON.stringify(item);
    const count = itemKeyCounts.get(baseKey) ?? 0;
    itemKeyCounts.set(baseKey, count + 1);
    return { item, key: `${baseKey}:${count}` };
  });

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
      {keyedItems.map(({ item, key }, index) => (
        <TreeEditor
          key={key}
          label={`Item ${index + 1}`}
          spec={itemSpec}
          tree={item}
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
    onTreeChanged({ ...map, [id]: field });
  }

  const children = spec.fields.map((fieldSpec) => (
    <TreeEditor
      key={fieldSpec.id}
      label={fieldSpec.name}
      spec={fieldSpec.spec}
      tree={map[fieldSpec.id]}
      onTreeChanged={(field) => {
        handleFieldChange(fieldSpec.id, field);
      }}
    />
  ));

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
    onTreeChanged({ ...map, _active, [_active]: map[_active] ?? null });
  }

  assert(map._active === null || typeof map._active === "string");
  const _active = map._active;

  function handleOptionChange(option: Tree) {
    assert(_active !== null);
    onTreeChanged({ ...map, [_active]: option });
  }

  const activeSpec =
    _active === null
      ? undefined
      : spec.options.find((option) => option.id === _active);
  assert(_active === null || activeSpec !== undefined);
  const activeTree = _active === null ? undefined : map[_active];

  return (
    <TreeTable.Node
      label={label}
      color={_active === null ? "red" : undefined}
      value={
        <Removable onRemove={onRemove}>
          <Select.Root value={_active} onValueChange={handleActiveChange}>
            <Select.Trigger />
            <Select.Content>
              {spec.options.map((optionSpec) => (
                <Select.Item key={optionSpec.id} value={optionSpec.id}>
                  {optionSpec.name}
                </Select.Item>
              ))}
            </Select.Content>
          </Select.Root>
        </Removable>
      }
    >
      {activeSpec !== undefined && (
        <TreeEditor
          label=""
          spec={activeSpec.spec}
          tree={activeTree ?? null}
          onTreeChanged={handleOptionChange}
        />
      )}
    </TreeTable.Node>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// RefTreeEditor
//

function RefTreeEditor({
  label,
  spec,
  tree,
  onTreeChanged,
  onRemove,
}: Readonly<TreeEditorProps<RefSpec>>) {
  assert(tree === null || typeof tree === "string");

  const { namespaces } = usePropertyEditorContext();
  const symbols = namespaces[spec.namespace] ?? {};
  const symbolEntries = Object.entries(symbols);

  return (
    <TreeTable.Node
      label={label}
      color={tree === null || symbols[tree] === undefined ? "red" : undefined}
      value={
        <Removable onRemove={onRemove}>
          <Select.Root
            value={tree}
            onValueChange={(value) => {
              onTreeChanged(value);
            }}
          >
            <Select.Trigger />
            <Select.Content>
              {symbolEntries.map(([symbol, path]) => (
                <Select.Item key={path} value={symbol}>
                  {symbol}
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
// Removable
//

interface RemovableProps {
  onRemove?: () => void;
  children?: ReactNode;
}

function Removable({
  onRemove,
  children,
}: Readonly<RemovableProps>): ReactNode {
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
