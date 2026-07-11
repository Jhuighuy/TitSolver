# BlueTit GUI — Project System Plan (Phases 2 & 3)

Date: 2026-07-11. Prerequisite (done): `tit::prop` follow-ups landed on this
branch (`78befe870..c37510a90`), full C++ suite green.

**Status: ALL PHASES (2a–2c, 3a–3d) implemented and verified end-to-end.**
The solver runs from a case directory consuming `case.yaml` (end time,
output cadence); the session is case-scoped; persisted state lives in
`userData` (the legacy repo-root file migrates automatically); the `case`
IPC service is live. The center of the window is a typed tab workspace
(welcome + N numbered viewport tabs, persisted layout, empty-workspace
screen, "+" menu); the timeline and view controls live inside each viewport
tab; viewport state is a per-tab factory bundle; the Setup pane renders the
case spec with materialized values, explicit-vs-default markers with reset,
per-field issue badges, and a Problems section. One deviation from
§Renormalization item 6: saving is never blocked — the authored tree always
round-trips safely, so materialization issues gate *running* instead of
saving. Dev helper: `TITGUI_CASE=<dir>` opens a case at startup.

Decisions already made: **YAML** case files; solver consumption via **minimal
argv wiring** (a few real parameters, the rest mocked for now); the
`gui-editors-dev` GUI commit is design reference only — its editors are
rebuilt on current idioms (typed IPC, jotai, tokens, options-driven
components).

---

## Phase 2 — Case model (main process + native)

A **case** is a directory the user owns:

```
my-dam-break/
├─ case.yaml        # authored property tree (sparse; see §Renormalization)
├─ particles.ttdb   # storage, written by the solver (rename to case.ttdb later)
└─ …assets (STL geometry, exports) later
```

### 2.1 C++: the case spec

- New small library target consumed by both the solver and the GUI bindings
  (e.g. `source/titwcsph/case/` → `titwcsph_case`): builds the WCSPH case
  `prop::Spec` (fluid, boundaries, integrator, output cadence, …) and owns
  the schema-version constant. Single source of truth — the old branch built
  the spec inside the GUI bindings, which is the wrong home once the solver
  reads it too.
- `titwcsph` accepts an optional case-file argument: loads the tree,
  validates against the spec, and consumes a _minimal honest subset_ (end
  time, output cadence); everything else stays hardcoded for now with a
  clear `TODO(case)` trail. No argument → current behavior, unchanged.

### 2.2 Native bindings (stateless, one-shot)

Following the `readFrame` lesson — JSON in, JSON out, no live handles:

```ts
caseSpec(): Promise<SpecJson>;                    // spec-json serialization
loadCaseTree(path: string): Promise<TreeJson>;    // YAML → tree → JSON
saveCaseTree(path: string, tree: TreeJson): Promise<void>;
materializeCase(tree: TreeJson): Promise<{
  tree: TreeJson;               // normalized (defaults filled, coerced)
  issues: Issue[];              // { code, path, message }
  namespaces: NamespaceTable;   // symbol → path, for reference editors
}>;
```

Config trees are tiny; serializing whole trees per call is simpler and safer
than the old branch's stateful `PropTreeObject` handles.

### 2.3 Main process

- **`CaseManager`** (`main/case.ts`): the open case (directory, authored
  tree, dirty flag), operations `newCase(dir)`, `openCase(path)`, `save()`,
  `close()`, and a **recents list**.
- **Persisted state moves to `userData`** (finally closing §2.1 of the
  proposals doc): app-level state (recents, theme, window geometry) in
  `app.getPath("userData")/persisted-state.json`; the hardcoded
  work-directory logic is deleted.
- **`SessionManager` becomes case-scoped**: constructed/reset by CaseManager
  with the case directory as `workDir`; storage path is
  `<case>/particles.ttdb`; the existing open-retry poll means storage that
  does not exist yet (before the first run) just works. No open case → no
  session; `session.*` IPC reports the empty state it already supports.
- **IPC**: new `case` service in the contract —
  methods `state`, `newCase`, `openCase` (dialog in main), `openRecent`,
  `save`, `close`, `getSpec`, `updateTree(tree, revision)` (returns the
  materialized document); events `caseChanged` (open/close/rename),
  `treeChanged` (materialized document push). Revision counter guards
  against stale writes, as in the old branch.
- Window title: `<case name> — BlueTit`.

### 2.4 Verification

Unit: CaseManager recents/dirty logic (vitest); spec/materialize round-trip
(C++ side is already covered by `tit/prop` tests; add a `titwcsph_case` spec
test). E2E: new case → edit → save → reopen → identical authored YAML; run
solver from the case directory; storage auto-appears via the retry poll.

---

## Phase 3 — VSCode-style workspace

### 3.1 Shell

The center of the window becomes a **tab workspace** (the existing generic
`Tabs` component, extended with icons + dirty markers):

```
┌ title bar ─────────────────────────────────────┐
│ rail │ [⌂ Welcome] [👁 Viewport] [+]            │
│      │                                          │
│ left │            active tab content            │
│ menus│                                          │
│      ├──────────────────────────────────────────┤
│      │ bottom panel (Logs / Output)             │
└──────┴──────────────────────────────────────────┘
```

- Tabs are **typed instances**: `{ id, kind: "welcome" | "viewport", … }`;
  new kinds (property editor as a tab? diff viewer?) slot in later.
- The left rail (Dashboard/Help/Settings + future Setup) and the bottom
  panel stay app-level chrome. **Timeline and view controls move inside the
  viewport tab** — they are per-viewport concerns.
- All tabs closable. **Empty workspace state**: a quiet centered screen
  listing openable tabs (“Welcome”, “Viewport” — the VSCode empty-editor
  pattern) instead of a dead area.
- Tab layout (open tabs, active tab) persists per window via
  `useWindowState`.

### 3.2 Welcome tab

Product mark, primary actions (**New Case…**, **Open Case…**), and the
**recents list** (name, path, last opened; click to open). Shown on first
launch and whenever no case is open; closable once a case is open.

### 3.3 Viewport tabs & the multi-viewport refactor

The one structural change: `state/viewport.ts` is currently module-level
atoms — fine for one viewport, wrong for N. Refactor to a factory:

- `createViewportState()` returns the atom bundle + `bindRenderer`; a tab
  registry maps viewport-tab id → its state bundle. Components receive the
  bundle via a small context provider at the tab root (atoms stay in the
  default store; the bundle is per-instance by construction).
- **Shared vs per-viewport**: storage/frames/frame-index stay global (one
  case, one timeline position); camera, field selection, coloring, tool
  mode, legend are per-viewport. Two viewports showing different fields of
  the same frame is the payoff.
- Ship order: do the factory refactor first with a single viewport tab
  (mechanical, low risk), enable “New Viewport” once stable.

### 3.4 Setup / property editor (bridging into the workspace)

The spec-driven editor (rebuilt from the old branch's design) lands as the
**Setup** entry — first as a left-rail pane, since panes already exist; it
can graduate to a tab kind later without API changes. Editors are composed
from existing components (`LabeledSelect`, `NumberInput`, `Switch`,
`Section`, `TreeTable`), driven entirely by `SpecJson`, with per-field
issue badges from the materialized document.

---

## Tree renormalization (“materialization”)

The core tension: spec validation _mutates_ the tree (fills defaults,
coerces types, sets variant `_active`, resolves symbols/references), edits
invalidate previous normalization, and the on-disk file should stay minimal
and diff-friendly while the editor shows a complete picture. Position:

1. **One normalizer, in one place.** Only the C++ spec normalizes, via
   `materializeCase` in the main process. The renderer never fills defaults
   or coerces — it displays materialized values and emits edits. Two
   implementations of defaulting logic is how the file and the UI drift.

2. **Two representations, explicitly.**
   - The **authored tree** (sparse): exactly what the user set; this is what
     `case.yaml` stores and what edits mutate.
   - The **materialized tree** (complete): authored + defaults + coercions;
     this is what the UI renders and what the solver consumes.
     Saving never writes materialized values: _denormalization is not
     stripping — it is simply never adding_. A value the user explicitly set
     stays in the file even if it equals the default (authored intent), so
     future default changes don't silently reinterpret old files, and diffs
     stay meaningful. The editor marks fields as _default_ vs _modified_
     (the VSCode settings-gear pattern), with “reset to default” deleting the
     authored node rather than writing the default value.

3. **Renormalization cadence.** Every committed edit → patch the authored
   tree → `materializeCase` → push `{tree, issues, namespaces}` to the
   renderer (debounced ~50–100 ms; trees are small, so this is cheap).
   Fields keep **local draft state and commit on blur/enter** (the
   `NumberInput` pattern we already use), so mid-typing values are never
   clobbered or “corrected” by a renormalization round trip, and a field
   showing an issue keeps the user's text, not a coerced substitute.

4. **Structural renormalization (variants).** Switching a variant's
   `_active` keeps the inactive branch's authored values **in memory** for
   the session (switching back loses nothing) but **omits inactive branches
   on save** — files describe the case, not the editing session. Unknown
   keys found on load are diagnosed (`unknown_field`) but preserved through
   save — forward compatibility over tidiness.

5. **Spec evolution.** `case.yaml` carries a schema-version key. On open,
   materialize-with-issues _is_ the migration check: missing new fields fall
   back to defaults silently; genuinely breaking changes get explicit
   migration steps keyed off the version. No implicit rewriting of the
   user's file on open — the file changes only on save.

6. **Saving with issues** is allowed for warnings but blocked for hard
   errors (the old branch blocked all; too strict — a half-configured case
   should be saveable, just not runnable). _Running_ the solver requires a
   clean materialization.

---

## Sequencing & risks

| Step | Contents                                                                         | Risk                                           |
| ---- | -------------------------------------------------------------------------------- | ---------------------------------------------- |
| 2a   | `titwcsph_case` spec lib + bindings (`caseSpec`/`load`/`save`/`materializeCase`) | low; prop APIs tested                          |
| 2b   | CaseManager + recents + userData persisted state + case IPC service              | medium: persisted-state migration              |
| 2c   | Session becomes case-scoped; solver argv wiring                                  | low; retry-poll already handles absent storage |
| 3a   | Workspace tab shell + welcome tab + empty state (single viewport tab)            | medium: layout restructure                     |
| 3b   | Viewport state factory (multi-viewport-ready, still one tab)                     | medium: touch-everything refactor, mechanical  |
| 3c   | Setup editor (spec-driven) with materialized documents                           | medium: editor breadth                         |
| 3d   | “New Viewport” (N viewports)                                                     | low once 3b lands                              |

Open questions parked (not blockers): storage filename (`particles.ttdb` →
`case.ttdb` needs a solver change), per-case UI state (camera etc.) in the
case directory, and whether Setup eventually becomes a tab kind.
