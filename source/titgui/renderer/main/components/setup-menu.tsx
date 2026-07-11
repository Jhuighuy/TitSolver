/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { IconDeviceFloppy, IconRestore } from "@tabler/icons-react";
import { useAtomValue } from "jotai";
import { type KeyboardEvent, type ReactNode, useState } from "react";

import { IconButton } from "~/renderer/common/components/button";
import { NumberInput, TextInput } from "~/renderer/common/components/input";
import { useMenuAction } from "~/renderer/common/components/menu";
import { Section } from "~/renderer/common/components/section";
import { Select } from "~/renderer/common/components/select";
import { Switch } from "~/renderer/common/components/switch";
import { Text } from "~/renderer/common/components/text";
import { Tooltip } from "~/renderer/common/components/tooltip";
import {
  caseDocumentAtom,
  caseSpecAtom,
  caseStateAtom,
  resetCaseValue,
  saveCase,
  setCaseValue,
} from "~/renderer/main/state/case";
import {
  type CaseDocument,
  type SpecJson,
  treeGetAt,
  treePathToString,
} from "~/shared/case";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
 * The Setup pane: the case editor, driven entirely by the case
 * specification. Values shown come from the materialized tree; edits patch
 * the authored tree and come back as a new materialized document.
 */
export function SetupMenu() {
  const caseState = useAtomValue(caseStateAtom);
  const spec = useAtomValue(caseSpecAtom);
  const document = useAtomValue(caseDocumentAtom);

  useMenuAction({
    name: "Save Case",
    icon: <IconDeviceFloppy />,
    disabled: caseState === null || !caseState.dirty,
    onClick: () => {
      void saveCase();
    },
  });

  if (caseState === null || document === null) {
    return (
      <div className="p-3">
        <Text color="subtle">Open a case to edit its setup.</Text>
      </div>
    );
  }

  if (spec === null || spec.type !== "record") {
    return (
      <div className="p-3">
        <Text color="subtle">The case specification is not available.</Text>
      </div>
    );
  }

  const problems = unmatchedIssues(spec, document);

  return (
    <div className="flex flex-col gap-3 p-2">
      {spec.fields
        .filter((field) => field.id !== "schema")
        .map((field) => (
          <SpecFieldEditor
            key={field.id}
            path={[field.id]}
            name={field.name}
            spec={field.spec}
            document={document}
          />
        ))}

      {problems.length > 0 && (
        <Section label="Problems">
          <div className="flex flex-col gap-1">
            {problems.map((issue) => (
              <Text key={`${issue.path}:${issue.message}`} color="danger">
                {issue.path}: {issue.message}
              </Text>
            ))}
          </div>
        </Section>
      )}
    </div>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

interface SpecFieldEditorProps {
  path: string[];
  name: string;
  spec: SpecJson;
  document: CaseDocument;
}

// One field of the specification, dispatched by kind. Records and variants
// recurse; scalars render an editor row.
function SpecFieldEditor({
  path,
  name,
  spec,
  document,
}: Readonly<SpecFieldEditorProps>) {
  const value = treeGetAt(document.materialized.tree, path);

  switch (spec.type) {
    case "record":
      return (
        <Section label={name}>
          <div className="flex flex-col gap-2">
            {spec.fields.map((field) => (
              <SpecFieldEditor
                key={field.id}
                path={[...path, field.id]}
                name={field.name}
                spec={field.spec}
                document={document}
              />
            ))}
          </div>
        </Section>
      );

    case "variant": {
      const activePath = [...path, "_active"];
      const active = treeGetAt(document.materialized.tree, activePath);
      const activeId = typeof active === "string" ? active : spec.default;
      const option = spec.options.find((other) => other.id === activeId);
      return (
        <Section label={name}>
          <div className="flex flex-col gap-2">
            <FieldScaffold label="Type" path={activePath} document={document}>
              <Select.Root
                value={activeId ?? ""}
                onValueChange={(id) => {
                  setCaseValue(activePath, id);
                }}
                options={spec.options.map(({ id, name: label }) => ({
                  value: id,
                  label,
                }))}
              >
                <Select.Trigger className="w-full" />
                <Select.Content />
              </Select.Root>
            </FieldScaffold>

            {option !== undefined && activeId !== undefined && (
              <SpecFieldEditor
                path={[...path, activeId]}
                name={option.name}
                spec={option.spec}
                document={document}
              />
            )}
          </div>
        </Section>
      );
    }

    case "bool":
      return (
        <FieldScaffold
          label={name}
          path={path}
          document={document}
          inline={
            <BoolEditor
              checked={value === true}
              onCheckedChange={(checked) => {
                setCaseValue(path, checked);
              }}
            />
          }
        />
      );

    case "int":
    case "real":
      return (
        <FieldScaffold label={name} path={path} document={document}>
          <NumberEditor spec={spec} value={value} path={path} />
        </FieldScaffold>
      );

    case "string":
      return (
        <FieldScaffold label={name} path={path} document={document}>
          <DraftTextInput
            value={typeof value === "string" ? value : ""}
            onCommit={(text) => {
              setCaseValue(path, text);
            }}
          />
        </FieldScaffold>
      );

    case "enum":
      return (
        <FieldScaffold label={name} path={path} document={document}>
          <Select.Root
            value={typeof value === "string" ? value : ""}
            onValueChange={(id) => {
              setCaseValue(path, id);
            }}
            options={spec.options.map(({ id, name: label }) => ({
              value: id,
              label,
            }))}
          >
            <Select.Trigger className="w-full" />
            <Select.Content />
          </Select.Root>
        </FieldScaffold>
      );

    case "array":
    case "symbol":
    case "ref":
      return (
        <FieldScaffold label={name} path={path} document={document}>
          <Text color="subtle">
            Fields of kind “{spec.type}” are not editable yet.
          </Text>
        </FieldScaffold>
      );
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

interface FieldScaffoldProps {
  label: string;
  path: string[];
  document: CaseDocument;

  /** Editor rendered on the label row (e.g. a switch). */
  inline?: ReactNode;

  /** Editor rendered under the label row. */
  children?: ReactNode;
}

// The label row of a field: the explicit-vs-default marker, the reset
// control, the editor, and the field's validation issues.
function FieldScaffold({
  label,
  path,
  document,
  inline,
  children,
}: Readonly<FieldScaffoldProps>) {
  // A field is "modified" when the authored tree holds an explicit value.
  const modified = treeGetAt(document.authored, path) !== undefined;
  const pathString = treePathToString(path);
  const issues = document.materialized.issues.filter(
    (issue) => issue.path === pathString,
  );

  return (
    <div className="flex flex-col gap-1">
      <div className="flex h-5 items-center justify-between gap-2">
        <Text color={modified ? "default" : "subtle"} truncate>
          {label}
        </Text>
        <div className="flex shrink-0 items-center gap-1">
          {modified && (
            <Tooltip content="Reset to default">
              <IconButton
                aria-label={`Reset ${label} to default`}
                onClick={() => {
                  resetCaseValue(path);
                }}
              >
                <IconRestore />
              </IconButton>
            </Tooltip>
          )}
          {inline}
        </div>
      </div>
      {children}
      {issues.map((issue) => (
        <Text key={issue.message} color="danger">
          {issue.message}
        </Text>
      ))}
    </div>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

interface BoolEditorProps {
  checked: boolean;
  onCheckedChange: (checked: boolean) => void;
}

function BoolEditor({ checked, onCheckedChange }: Readonly<BoolEditorProps>) {
  return <Switch checked={checked} onCheckedChange={onCheckedChange} />;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

interface NumberEditorProps {
  spec: Extract<SpecJson, { type: "int" | "real" }>;
  value: unknown;
  path: string[];
}

function NumberEditor({ spec, value, path }: Readonly<NumberEditorProps>) {
  const min = spec.min ?? Number.NEGATIVE_INFINITY;
  const max = spec.max ?? Number.POSITIVE_INFINITY;

  // A value that violates the spec (possible while the case has issues)
  // cannot be displayed by the number input; show it as a placeholder.
  const valid =
    typeof value === "number" &&
    min <= value &&
    value <= max &&
    (spec.type === "real" || Number.isInteger(value));

  const unit = "unit" in spec ? spec.unit : undefined;
  return (
    <div className="flex items-center gap-2">
      <NumberInput
        className="min-w-0 flex-1"
        type={spec.type}
        min={spec.min}
        max={spec.max}
        value={valid ? value : null}
        placeholder={valid ? undefined : String(value)}
        onValueChange={(next) => {
          if (next !== null) setCaseValue(path, next);
        }}
      />
      {unit !== undefined && (
        <Text color="subtle" className="shrink-0">
          {unit}
        </Text>
      )}
    </div>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

interface DraftTextInputProps {
  value: string;
  onCommit: (value: string) => void;
}

// Text input with a local draft, committed on blur or Enter — mid-typing
// values are never clobbered by a renormalization round trip.
function DraftTextInput({ value, onCommit }: Readonly<DraftTextInputProps>) {
  const [draft, setDraft] = useState<string | null>(null);

  const handleKeyDown = (event: KeyboardEvent<HTMLInputElement>) => {
    if (event.key === "Enter") event.currentTarget.blur();
    if (event.key === "Escape") setDraft(null);
  };

  return (
    <TextInput
      value={draft ?? value}
      onValueChange={setDraft}
      onBlur={() => {
        if (draft !== null && draft !== value) onCommit(draft);
        setDraft(null);
      }}
      onKeyDown={handleKeyDown}
    />
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Issues that do not belong to any rendered editor row (e.g. unknown
// fields), surfaced in the Problems section.
function unmatchedIssues(spec: SpecJson, document: CaseDocument) {
  const leafPaths = new Set<string>();
  collectLeafPaths(spec, [], document, leafPaths);
  return document.materialized.issues.filter(
    (issue) => !leafPaths.has(issue.path),
  );
}

function collectLeafPaths(
  spec: SpecJson,
  path: string[],
  document: CaseDocument,
  out: Set<string>,
) {
  switch (spec.type) {
    case "record":
      for (const field of spec.fields) {
        collectLeafPaths(field.spec, [...path, field.id], document, out);
      }
      break;
    case "variant": {
      out.add(treePathToString([...path, "_active"]));
      const active = treeGetAt(document.materialized.tree, [
        ...path,
        "_active",
      ]);
      const activeId = typeof active === "string" ? active : spec.default;
      const option = spec.options.find((other) => other.id === activeId);
      if (option !== undefined && activeId !== undefined) {
        collectLeafPaths(option.spec, [...path, activeId], document, out);
      }
      break;
    }
    case "bool":
    case "int":
    case "real":
    case "string":
    case "enum":
    case "array":
    case "symbol":
    case "ref":
      out.add(treePathToString(path));
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
