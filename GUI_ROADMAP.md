# BlueTit GUI — Roadmap

Date: 2026-07-12. Scope: `source/titgui` (+ the case/prop stack it sits on).
Successor to `GUI_IMPROVEMENT_PROPOSALS.md` (2026-07-05) and
`PROJECT_SYSTEM_PLAN.md` — everything actionable in those documents has
shipped; this one is about what comes next.

---

## 1. Where the codebase stands

The structural work is done and holding: typed zod-validated IPC contract,
jotai state with a per-viewport factory, one token vocabulary, case-scoped
sessions, a spec-driven Setup editor on the authored/materialized document
model, electron-store persistence, a native app menu, and a test story
(49 vitest + 6 Playwright-Electron, all under `./build/build.sh --test`,
lcov to Codecov/Sonar). ~15.5 kLoC across four runtime contexts.

The consequence: the codebase no longer needs rescue work. The items below
are _growth_ — most of the value now comes from product features, with a
short list of engineering debts worth paying before they compound.

---

## 2. Product features

### 2.1 Solver configurability (highest product value)

The case spec exists end-to-end, but `titwcsph` honestly consumes only
`end_time` and `output.interval`; geometry, fluid, kernel, EOS, and
integrator remain hardcoded. Dismantling that is the single biggest step
toward a usable product:

- Wire the remaining spec parameters (gravity, density, viscosity, dam
  geometry, spacing) through `load_case` — mostly mechanical.
- Runtime selection of kernel / EOS / integrator needs a C++ dispatch layer
  over the current compile-time templates; grow the spec's variants
  (`time_integration`, `kernel`, …) together with it.
- Grow `titwcsph_case` toward arrays with symbols/references (materials,
  bodies, boundary conditions) — the prop library already supports them and
  they are unit-tested; only the WCSPH spec and the editor lack them.
- The prop **unit system is a stub** — real conversions (display unit vs SI
  storage) surfaced in the Setup editor would remove a whole class of user
  error.

### 2.2 Case & workspace UX

- **Unsaved-changes guard**: closing a dirty case (Close Case, opening
  another case, quitting the app) silently discards edits today. Needs the
  standard save/discard/cancel prompt, wired in CaseManager consumers and
  `before-quit`.
- **Dirty markers in the chrome**: the Setup pane's Save button knows about
  dirty state; the tab bar and macOS `documentEdited` dot do not.
- **Save policy refinement**: save currently writes the authored tree
  verbatim; the renormalization plan calls for omitting inactive variant
  branches on save (files describe the case, not the editing session).
- **Custom title bar is stale**: it renders `document.title` once at mount;
  the native title updates on case open but the in-window one does not.
  Make it follow `caseStateAtom`.
- Recents polish: remove/pin entries, drag-and-drop a case folder onto the
  window, dock-menu recents on macOS, `case.yaml` file association.
- `particles.ttdb` → `case.ttdb` rename (needs the solver-side change;
  parked since the project-system plan).
- Per-case UI state (camera, field selection, workspace layout) stored in
  the case directory, so reopening a case restores the view.

### 2.3 Timeline & playback

- **Physical time axis**: `frameTimesAtom` already carries every frame's
  time (shown as the `t = …` readout). The axis itself still labels frame
  indices; mapping ticks to nice _time_ steps (non-uniform frame spacing)
  is the natural next step.
- **Playback speed control** (1×/2×/4×, or frames-per-second): trivially
  slots into the request-driven loop (`FRAME_INTERVAL_MS` is a constant
  today).
- Solver **progress**: end time is known from the case, the last frame time
  from storage — a progress bar / ETA in the Dashboard (and perhaps a quiet
  in-viewport pill during runs) is now computable.

### 2.4 Viewport & analysis

- **Selection should answer questions**: today it only counts particles.
  Show per-selection statistics (min/max/mean of the colored field), and a
  probe/inspect tool (click a particle → all field values).
- **Geometry import**: STL walls with visibility toggles existed in the
  `titapp` prototype and are worth porting once geometry participates in
  the solve.
- Viewport screenshot/record export (the WebGL canvas is right there).
- Camera bookmarks / standard views beyond the view cube; configurable
  mouse bindings for the (deliberately kept) custom `CameraController`.
- Frame data export refinement: per-frame and per-selection export, CSV
  alongside HDF5 (the Export button today exports the whole series only).

### 2.5 Runs & series

`wcsph.cpp` sets `set_max_series(1)` and the session always opens
`lastSeries()` — every run overwrites history. A **run history** (keep N
series, browse/compare, annotate) would make the Dashboard genuinely
useful. This is also the natural content for the **left-rail "Storage"
item, which is currently an empty stub** — either build the series/storage
browser there or remove the stub.

### 2.6 Settings & distribution

- Settings pane holds exactly one setting (theme). Obvious additions:
  default background, playback speed, telemetry interval, camera bindings.
  The persistence and schema plumbing is already in place.
- Distribution track, when the time comes: code signing + notarization
  (also unlocks the `role: "help"` menu quirk), auto-update, crash
  reporting for the main process, and a decision on Linux/Windows packaging
  (the Installation resolver already models the layouts).
- i18n is untested territory (all strings inline English); decide early
  whether it matters, because retrofitting extraction is dull work that
  only grows.

---

## 3. Engineering improvements

### 3.1 Testing depth (best effort-to-value)

- **Unlock state-layer unit tests**: `renderer/common/ipc.ts` throws at
  import when the preload bridge is absent, so the storage/solver/playback/
  case state modules — the most logic-dense renderer code — are untestable
  in vitest. A small injection seam (test transport for the generated
  client) would open all of them; coverage sits at ~12% statements and this
  is the honest way to raise it.
- **E2E growth**: a solver-run round trip (Run → frames appear → Stop) is
  feasible inside CTest where `titwcsph` is guaranteed built; workspace tab
  manipulation; persistence across relaunch; a dark-theme pass. Playwright
  screenshot assertions could replace today's manual screenshot ritual.
- **CI**: confirm `titgui/e2e` actually executes on the macOS runners and
  whether ubuntu images ship `xvfb-run` (the registration degrades to skip,
  so this is silent). Start tracking `package-lock.json` (the whitelist
  `.gitignore` currently excludes it) for reproducible CI installs.

### 3.2 Code health (small, concrete)

- **Split `main/main.ts` (~430 lines)**: the `Application` class now does
  lifecycle + session reset + dialogs + menu + every IPC handler. Extract
  per-service handler modules (`main/handlers/{case,session,help}.ts`);
  the contract types make this mechanical.
- **Componentize the Setup editor** (`setup-menu.tsx`, 439 lines) before
  array/symbol/reference editors land — an `editors/` directory with one
  file per spec kind, plus the future "Setup as a tab kind" graduation.
- **Validate at the bindings boundary**: `bindings/index.ts` casts
  `JSON.parse` results (`as SpecJson`, `as MaterializedCase`); the zod
  schemas exist in `shared/case.ts` — parse instead of cast.
- **Accessible labels for Setup fields**: inputs are reachable only by
  DOM order (the E2E resorts to `getByRole("textbox").nth(n)`);
  `FieldScaffold` should wire `aria-label`/`htmlFor` to its editor.
- **Number formatting**: Base UI NumberField renders ~3 fraction digits
  (0.0075 displays as 0.008) — derive format precision from the spec, or
  use significant digits.
- Toast/log rate limiting for repeated errors (a failing poll can spam).

### 3.3 Performance (measure first, none urgent)

- Frame delivery is one native call + one structured clone, LRU-cached with
  prefetch — fine at current sizes. If particle counts grow 10×: metadata-
  only reads for UI, transferables over structured clone, and viewport
  LOD/decimation, in that order, each behind a measurement. (True zero-copy
  is off the table: Electron's V8 memory cage forbids external
  ArrayBuffers.)
- `readFrame` re-resolves `lastSeries()` per call — harmless now, folds
  into the run-history work (§2.5) where the open series becomes explicit.

### 3.4 Housekeeping

- The `titapp`/`titback` prototype (branch `gui-fable`) is superseded;
  harvest what is still valuable (STL import flow, playback speed UI,
  DiveCAE styling experiments) into issues and archive the branch, so the
  two-codebase question never reopens.
- `manual/` has no content about the case system yet; the help window is
  wired and waiting.
- Spec evolution is designed (schema version + materialize-as-migration)
  but has never been exercised — write the v1→v2 migration test before the
  first real schema change, not during it.

---

## 4. Suggested order

| Horizon        | Items                                                                                                                               | Rationale                                                             |
| -------------- | ----------------------------------------------------------------------------------------------------------------------------------- | --------------------------------------------------------------------- |
| **Now**        | Unsaved-changes guard; dirty markers + reactive title bar; state-layer test seam; `main.ts` split                                   | Small, protects user data, unblocks testing before features pile on   |
| **Next**       | Solver configurability §2.1 (params first, dispatch second); Setup editor componentization + array/symbol editors; unit conversions | The product's core value; editor work and spec growth go hand in hand |
| **Then**       | Run history + Storage pane; timeline time axis + playback speed; solver progress; selection statistics                              | Analysis workflows on top of the (by then) configurable solver        |
| **Later**      | Geometry import; per-case UI state; settings growth; distribution (signing, updates, crash reporting); i18n decision                | Product maturity and shipping concerns                                |
| **Continuous** | E2E growth, CI verification, lockfile tracking, manual content, dependency-policy upgrades                                          | Keeps the quality bar where it now is                                 |

The one deliberate non-goal, restated from the review: no second rewrite.
The architecture earned its keep this cycle; everything above builds on it.
