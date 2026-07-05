# GUI (`source/titgui`) — Code Review & Improvement Proposals

Date: 2026-07-05. Scope: everything under `source/titgui/` (main process, preload,
renderer, common components, Three.js visual layer, native bindings, build
config). Lint (`tsgo`, `oxlint`, `oxfmt`) is currently clean, so this review
concentrates on architecture, correctness, and design-language consistency —
the things linters cannot see.

---

## 1. What is already good

Worth stating explicitly, because these should be preserved through any
refactor:

- **Security posture is right.** `contextIsolation: true`, `sandbox: true`,
  `nodeIntegration: false`, webview preferences forcibly sanitized in
  `main/help.ts`, window-open requests denied. This is better than most
  Electron apps.
- **Strict tooling.** Type-aware oxlint with `pedantic`/`suspicious` as errors,
  `strict` TS, zero-warning policy, React Compiler enabled.
- **Native bindings are disciplined.** All storage work is off the JS thread
  via `PromiseWorker`, arguments are checked, 64-bit integers are converted
  deliberately.
- **Consistent file hygiene.** License headers, section separators, `~/`
  imports, co-located component styles — the codebase has a recognizable voice.
- **The Three.js layer is cleanly isolated** behind `Renderer` /
  `Particles` interfaces and knows nothing about React.

---

## 2. High-priority findings (correctness & robustness)

### 2.1 The app is hard-wired to a single directory and a single file

`main/main.ts:71` derives `workDir` as `<install>/../..` and
`main/session.ts:133` hard-codes `particles.ttdb`; `persisted-state.json` is
written into the same directory instead of `app.getPath("userData")`. There is
no notion of a _case/project_: no "Open…", no recents, no way to view a second
dataset without restarting. This is the single biggest product-level
limitation and it shapes several other problems below (storage lifecycle,
refresh semantics).

**Proposal:** introduce a `Case` domain object in the main process (directory

- storage file + solver settings), an `app:open-case` IPC flow with a recents
  list, and move app-level persisted state to `userData`. Everything currently
  called "session" becomes scoped to the open case.

### 2.2 Startup failure path is an unhandled rejection

`Application.run()` does `void this.onReady()`. `SessionManager.start()`
awaits `openStorage(...)`, which rejects if `particles.ttdb` does not exist —
the rejection is swallowed, no window opens, and the user gets a silent dead
process. Similar fragility exists downstream: if `globalThis.session` is
absent the IPC handler returns `undefined` and
`renderer/main/components/storage.tsx:62` hits `assert(numFrames >= 0)` inside
a `.then`, producing an unhandled rejection and a permanently empty UI.

**Proposal:** a deliberate error strategy (see §5.3). At minimum: `try/catch`
in `onReady` with a dialog + graceful degradation ("no storage open" state in
the renderer instead of asserts on missing data).

### 2.3 Path traversal in the `help://` protocol handler

`main/help.ts:355-361` rejects paths that _start with_ `..`, but embedded
segments pass: `help://manual/a/../../secret` resolves outside
`manualPath` and `net.fetch(pathToFileURL(...))` will happily serve it into
the webview. Severity is moderate (local app, local files), but it is exactly
the kind of check that must be watertight.

**Proposal:** resolve first, then verify containment:

```ts
const resolved = path.resolve(this.rootPath, relativePath);
if (path.relative(this.rootPath, resolved).startsWith("..")) throw ...;
```

### 2.4 Unbounded solver output in React state

`solver.tsx:59` accumulates stdout/stderr into a single string with
`prev + response.data`. Every chunk re-renders the provider subtree and string
concatenation is O(n²) over a run; a long simulation with verbose logging will
degrade the whole UI. The dashboard renders the entire text in one `<pre>`.

**Proposal:** cap the buffer (ring buffer of lines), keep it _outside_ React
(store/signal, subscribe at leaf), and render through a virtualized list or a
real terminal widget (xterm.js) — which would also give the "Terminal" menu
stub (`app.tsx:69`) a purpose.

### 2.5 Playback issues frame requests faster than they can complete

`timeline.tsx:74` drives playback with `setInterval(…, 1000/60)`; each tick
calls `requestFrame`, which triggers a full IPC round trip → N per-field
native calls → HDF5 read → copy → structured clone. The `requestID` guard in
`storage.tsx` only discards stale _responses_; requests are still issued every
16 ms regardless of completion, so during playback the native thread pool is
saturated re-reading frames that will be thrown away.

**Proposal:** request-driven playback (`await` the frame, then schedule the
next tick), plus the data-layer changes in §4.

### 2.6 Solver process management races

`SolverManager.run` (`main/session.ts:48`) begins with
`assert(this.child === undefined)` — a double `runSolver` IPC call (double
click, replayed event) crashes the main process instead of being ignored.
Renderer-side `SolverProvider` keeps its own `isSolverRunningRef` mirror and
asserts on it too. The running state lives in three places (child handle, ref,
React state).

**Proposal:** make `run()` idempotent (return early / reject politely), make
the main process the single source of truth, and have the renderer treat
`isSolverRunning` as server state (query + push event), not as an asserted
invariant.

---

## 3. Architecture: the IPC layer

Adding one IPC method today touches **five** files: `shared/channels.ts`
(string constant), `main/main.ts` (handler with untyped `args`),
`preload/preload.ts` (hand-written wrapper), `shared/vite-env.d.ts` (global
API type), and the call site. Only the help service has a shared interface
(`satisfies HelpService`); `theme`, `windowState` and `session` are typed
once in `vite-env.d.ts` and trusted blindly. Except for solver events, nothing
is validated at the boundary — the zod schemas in `shared/storage.ts` exist
but are never `parse`d.

Every renderer call site does `globalThis.session?.foo()` — if the preload
failed to run, features silently no-op instead of failing loudly.

**Proposal:** one declarative, typed IPC contract as the single source of
truth:

```ts
// shared/ipc.ts
export const ipc = defineIpc({
  session: {
    frameCount: { result: z.number().int().nonnegative() },
    frame:      { args: z.tuple([z.number().int()]), result: frameSchema },
    ...
  },
  ...
});
```

with two small generic helpers: `registerIpcHandlers(ipc, impl)` for the main
process and `createIpcClient(ipc)` for the preload. Channel names are derived,
arguments and results are validated once, and the renderer global becomes a
non-optional, fully typed object (fail fast at startup if missing). If you
prefer not to build this, `electron-trpc` provides the same guarantees
off-the-shelf; but the hand-rolled version is ~100 lines and dependency-free.

This also removes the `ipcMain.removeHandler(...)` boilerplate repeated before
every `handle` in `main.ts` (a hot-reload workaround that suggests handler
registration should be table-driven anyway).

---

## 4. Architecture: the data layer (frames)

Fetching one frame currently costs:

1. `frame.fields()` — 1 native async call;
2. per field: `frame.field(name)` + `field.type()` + `field.data()` — **3 N**
   native calls, each acquiring the storage lock and enqueueing a worker;
3. a `memcpy` into a fresh `ArrayBuffer` per field (`bindings/storage.cpp:402`);
4. a structured-clone of all arrays across the IPC boundary;
5. a rebuild into `FieldMap` in the renderer.

There is no caching: stepping back one frame re-reads everything from HDF5.

**Proposals (independent, in order of value):**

1. **One-shot native `readFrame(index)`** returning
   `Record<string, { type, data }>` in a single worker — removes the 3N+1
   chatter and lock churn. The fine-grained `Storage/Series/Frame/Field` wrap
   classes can remain for other uses, but the hot path should be one call.
2. **An LRU frame cache + neighbor prefetch in the main process** (or
   renderer) keyed by `(seriesID, frameIndex)`; playback and scrubbing become
   cache hits.
3. **Frame metadata separated from bulk data** so UI concerns (field list,
   ranks) don't require re-reading arrays.
4. Longer term, if frame sizes grow: move bulk transfer off structured clone
   (e.g. `postMessage` with transferables via `MessageChannelMain`, or a local
   socket protocol). Measure first; for < ~10⁶ particles structured clone may
   be acceptable once caching exists.

Also: `series` is re-resolved via `lastSeries()` on _every_ call
(`main/session.ts:148`), and "refresh" (`storage.tsx refresh()`) never reopens
the storage, so a series created by a new solver run after startup is only
picked up by accident of `lastSeries`. Make the open series explicit state
with a real reload path, and push a "storage changed" event to the renderer
when the solver writes new frames — today the user must click Refresh
manually.

---

## 5. Architecture: renderer state management

### 5.1 Four state systems in one renderer

The renderer currently mixes:

1. React context providers with `useState`/refs (`StorageProvider`,
   `SolverProvider`) including hand-rolled race protection (session/request ID
   refs + `useEffectEvent`);
2. a **bespoke signals library** (`renderer/common/signals.ts`) driving
   `ViewportModel`;
3. React Query as a cache for IPC-backed persisted values (`use-window.ts`,
   `use-theme.ts`, `use-session.ts`);
4. window-persisted state round-tripping through the main process.

Each is locally reasonable; together they mean every feature answers "where
does state live?" differently. The custom signals code is the most concerning:
it re-implements a solved problem (~125 lines with `dispose()` APIs that are
never called), and `ScopedSignal`'s string-concatenated scope keys
(`keySources.map(get).join(":")`) plus the `autoColorRange` push logic in
`viewport.tsx:497-531` form a dependency graph that takes real effort to
verify.

**Proposal:** standardize on **one** primitive. Given the shapes already in
the code, [jotai](https://jotai.org) is the closest drop-in: `atom` ≈
`signal`, derived atoms ≈ `derived`, `atomFamily` ≈ `scoped`, and
`useAtomValue` replaces `useSignalValue` — while staying usable outside React
via the store API (which `ViewportModel` needs for its Three.js
subscriptions). Zustand is a fine alternative if you prefer a single-store
model. Keep React Query only for genuinely async server-ish state (frames), or
fold that into the same store; don't keep both patterns for the same kind of
data.

### 5.2 The Viewport ↔ model ↔ controls bridge

`Viewport` reads ~19 signals with `useSignalValue` and forwards each value
_and_ a setter closure into `ViewControls`, which is a 790-line component with
a ~34-prop interface, then re-splits them into four sub-components. The two
"Color Metric" blocks for rank-1 and rank-2 fields
(`view-controls.tsx:577-689`) are near-identical 55-line copies.

**Proposal:**

- Pass the model (or its atoms) down via context/props and let each control
  subscribe to exactly what it needs. The prop explosion, and most of
  `viewport.tsx`'s wiring boilerplate, disappears.
- Extract a generic `LabeledSelect`/`EnumSelect` (label + icon + options)
  — nearly every control in `view-controls.tsx` is the same
  `<Text color="subtle">` + `<Select.Root>` pattern — and a single
  parameterized `ColorMetricSelect` for both ranks.

### 5.3 Error handling and logging strategy

Current behavior is a patchwork: blocking `dialog.showMessageBoxSync` in the
main process, `assert` (throw) for anything unexpected in the renderer
(including _data_ conditions like a missing `rho` field in
`fields.ts:95`), a renderer-only `Logger` that nothing global feeds into, and
silent optional-chaining no-ops at IPC boundaries. There are no React error
boundaries, so any of the many `assert`s in render paths blanks the window.

**Proposal:**

1. Adopt a rule: `assert` is for programmer invariants only; anything caused
   by data, files, processes or the user must surface as a typed error and a
   visible-but-recoverable UI state (toast/banner + logged detail).
2. Add error boundaries around the viewport and each menu pane
   (`react-error-boundary` is fine), plus `window.onerror` /
   `onunhandledrejection` → `logger`.
3. Use `electron-log` (or a thin equivalent) in the main process instead of
   sync dialogs for non-fatal problems; dialogs only for actionable decisions.
4. Validate at boundaries with the zod schemas that already exist
   (`frameSchema` is currently dead code — either parse with it or delete it).

---

## 6. Design system & design-language consistency

### 6.1 Two competing theming/token mechanisms

`renderer/index.css` defines a semantic token set (`--bg-1..6`, `--fg-1..5`,
`--accent-*`, `--chrome-*`) keyed off `prefers-color-scheme` — good. But
`components/classes.ts` (`chrome`, `surface`, `hoverSurface`) bypasses it with
raw palette classes (`bg-slate-200 dark:bg-slate-700`), and hardcoded colors
leak elsewhere:

- `main/window.ts:226` duplicates `--bg-1` as literal hex with a "keep in
  sync" comment;
- `view-selection.tsx:217` uses `rgb(59,130,246)` — that is **blue-500**,
  while the app accent is **indigo**;
- `spheres.ts`/`glyphs.ts` selection color `[0.99, 0.76, 0.18]` is another
  ad-hoc accent;
- warning/error colors are inline (`logs-menu.tsx:100-104`,
  `text.tsx` `red` variant) with no `--danger`/`--warning` tokens.

**Proposal:** one token vocabulary, used everywhere. Extend the CSS variables
with semantic status tokens (`--danger-fg`, `--warning-fg`, `--selection`,
…), rewrite `chrome`/`surface` in terms of tokens, read
`--bg-1`/`--selection` at runtime (via `getComputedStyle`) for the window
background and the Three.js selection uniform so they can never drift.

### 6.2 The `Box`/`Flex` styling props system

`layout.tsx` implements a Radix-Themes-style prop API (`p`, `mx`, `size`,
`height="9"`, …) that compiles to **inline styles**, with `sp()` multiplying
bare numbers by 4px. Consequences:

- Three unit systems coexist in the same prop (`height="9"` → 36px,
  `height="10px"`, `height="100%"`), and `left={"calc(10px * …)"}` in
  `timeline.tsx` sidesteps it entirely.
- It duplicates what Tailwind (already a dependency) does with classes, but
  loses hover/responsive variants, dev-tools ergonomics and the shared design
  scale; half the codebase styles via `className`, the other half via props —
  two design languages in one renderer.

**Proposal:** pick one. Given Tailwind v4 + cva are already in place, the
industry-standard path is: components take `className` (merged via `cn`),
layout is expressed in Tailwind utilities, and `Box`/`Flex` either disappear
or shrink to trivial `div`/`flex div` wrappers. This is a mechanical but
codebase-wide migration — good candidate for one focused PR per directory.

### 6.3 Component API smells

- **`Select` walks React children** (`select.tsx:53-76`) to build a
  value→label registry — this breaks the moment items are wrapped in
  fragments-from-components, `memo`, or rendered lazily. Base UI's Select
  supports an `items` prop for exactly this; alternatively pass options as
  data (`options={[{value, label, icon}]}`), which would also simplify the
  dozens of call sites in `view-controls.tsx`.
- **`Menu` introspects `children[].props`** (`menu.tsx:129-204`) and persists
  the **active item as an index** validated against the _current_ number of
  children — reordering menu items silently corrupts users' persisted UI
  state. Use stable string IDs (`name`) as the persisted key and a
  data-driven `items` array instead of element inspection. The pane close
  button is a stub (`onClick={undefined /** @todo */}`).
- **`Tabs.Tab` takes `value: unknown`** and callers `assert(typeof value ===
"number")` on the way back out — make `Tabs` generic over the value type.
- **Copy-paste artifacts:** in `logs-menu.tsx` the "Save Logs" action is named
  `stopAction` and "Clear Logs" is `runAction` (copied from
  `dashboard-menu.tsx`).
- **Accessibility:** menu triggers are clickable `Flex` (div) elements with
  `aria-label` but no `role="button"`, no `tabIndex`, no keyboard activation;
  timeline ticks likewise. The `Resizable` handle shows the right pattern
  (role/aria/keyboard) — apply it consistently, or render real `<button>`s.

### 6.4 Timeline scalability & semantics

`timeline.tsx` renders `10^⌈log10(numFrames)⌉ + 1` tick `Box`es — 10 001 DOM
nodes with click handlers for a 1 001-frame series, most representing frames
that don't exist. It also shows only frame indices, not physical time.

**Proposal:** rewrite the strip as a single `<canvas>`/`<svg>` scale (one
element, computed ticks) with a slider-like interaction model
(`role="slider"`, arrow-key support), sized to actual `numFrames`, and leave
room for a time axis once frame timestamps are exposed by storage.

---

## 7. Native bindings & main-process details

- **Duplicate type definitions:** `bindings/storage.d.ts` and
  `shared/storage.ts` both define `Kind`, `Type`, `NumericArray` by hand.
  Derive one from the other (bindings types imported into the shared zod
  schemas via `z.custom`/`satisfies`), so the C++ ↔ TS contract lives in one
  file.
- `FieldWrap::data` copies into a fresh `ArrayBuffer` per call; with the
  one-shot `readFrame` (§4) consider `Napi::ArrayBuffer::New(env, data, len,
finalizer)` over the vector's own memory to remove the second copy.
- `PersistedState` is saved **only** in `will-quit` — a crash loses all
  window/UI state. Debounced save on change (or `electron-store`, which does
  atomic writes for you) is more robust; `withPrefix` instances silently
  can't `save()` (assert) — an API wart worth removing by design.
- `useWindowState`'s optimistic mutation stores `previous` in `onMutate` but
  has no `onError` rollback.
- macOS conventions: `window-all-closed` → `quit()` is un-mac-like, and there
  is no application menu wiring (`Menu.setApplicationMenu`), so standard
  shortcuts/menus are whatever Electron defaults to. Restored window bounds
  are not validated against current display bounds (window can reappear
  off-screen after a monitor change).
- `CameraController` is a solid custom implementation, but pan-on-left-drag /
  rotate-on-right-drag with no modifier alternatives is unusual for CAE tools;
  consider making bindings configurable and compare against
  `ArcballControls`/`OrbitControls` from three-stdlib before maintaining more
  interaction code.

---

## 8. Testing & process (the biggest structural gap)

There are **zero tests** for the GUI: no unit tests, no component tests, no
E2E, and no `test` script in `package.json`, while the C++ side of the repo
has a full CTest culture. Much of the code is eminently testable _today_
(pure modules: `fields.ts`, `color-map.ts`, `camera.ts`,
`particle-selection.ts`, `utils-math.ts`, `signals.ts`, `HelpSessionModel`,
`PersistedState`).

**Proposal:**

1. **Vitest** for pure logic + hooks (it shares Vite config); start with the
   modules above and every bug class found in this review (path traversal,
   scoped-signal semantics, field modifier math).
2. **Playwright's Electron driver** for a smoke E2E: app launches, storage
   loads, frame renders, solver run/stop round-trips. Even one such test
   catches the §2.2 class of silent startup death.
3. Register both under `tests/` with `add_tit_test(...)` so
   `./build/build.sh --test` covers the GUI like everything else.
4. Keep `tsgo` for speed, but consider running stable `tsc` in CI as the
   arbiter until tsgo leaves preview — the entire type-safety story currently
   rests on a `-dev.20260701` compiler build.
5. The stack deliberately rides the bleeding edge (React 19 `useEffectEvent`
   - `<Activity>`, Electron 43.0.0, Vite 8, Tailwind 4.3). That's a valid
     choice for a pre-release product, but write it down as policy (e.g. in
     `titgui/README.md`, which is currently three lines) including the upgrade
     cadence (`npm run upgrade` exists, but nothing documents when/how it's
     exercised or what must be re-verified after).

---

## 9. Prioritized roadmap

| Priority               | Item                                                                                                                                                               | Sections         |
| ---------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------ | ---------------- |
| **P0 — correctness**   | Fix `help://` containment check; handle `onReady` failure; make solver run idempotent; "no storage" UI state instead of asserts                                    | §2.2, §2.3, §2.6 |
| **P0 — foundations**   | Typed IPC contract layer (validation + generated preload); delete per-channel boilerplate                                                                          | §3               |
| **P1 — data path**     | One-shot native `readFrame`; frame LRU cache + prefetch; request-driven playback; storage-changed push events                                                      | §2.5, §4         |
| **P1 — state**         | Replace bespoke signals + provider patchwork with one store (jotai/zustand); dissolve `ViewControls` prop explosion                                                | §5.1, §5.2       |
| **P1 — robustness**    | Error strategy: boundaries, toasts, `electron-log`, bounded solver output (xterm.js)                                                                               | §2.4, §5.3       |
| **P2 — design system** | Single token vocabulary (incl. status/selection tokens, runtime-read for native/Three colors); retire `Box`/`Flex` inline-style props in favor of Tailwind classes | §6.1, §6.2       |
| **P2 — components**    | Data-driven `Select`/`Menu` APIs; stable persisted keys; generic `Tabs`; a11y pass on clickable divs; canvas-based timeline                                        | §6.3, §6.4       |
| **P2 — product**       | Case/project concept, Open/recents, persisted state in `userData`, macOS app menu & lifecycle conventions                                                          | §2.1, §7         |
| **Continuous**         | Vitest + Playwright-Electron wired into `add_tit_test`; stable `tsc` in CI; expand `titgui/README.md` into an architecture map                                     | §8               |

### Suggested library adoptions

| Library                                               | Replaces                                                 | Why                                                                       |
| ----------------------------------------------------- | -------------------------------------------------------- | ------------------------------------------------------------------------- |
| `jotai` (or `zustand`)                                | `renderer/common/signals.ts`, provider/ref patchwork     | Maintained, React-integrated, works outside React for the Three.js bridge |
| Hand-rolled `defineIpc` (~100 LoC) or `electron-trpc` | `channels.ts` + manual preload + `vite-env.d.ts` globals | Single typed, validated IPC contract                                      |
| `electron-log`                                        | `dialog.showMessageBoxSync` for non-fatal errors         | Non-blocking, file-backed, main+renderer transport                        |
| `electron-store` (optional)                           | `PersistedState`                                         | Atomic writes, schema support, save-on-change                             |
| `react-error-boundary`                                | nothing (gap)                                            | Recoverable UI on render-path failures                                    |
| `@xterm/xterm`                                        | `<pre>{solverOutput}</pre>`                              | Bounded, fast console; fills the "Terminal" stub                          |
| `vitest`, `@playwright/test`                          | nothing (gap)                                            | Unit/component + Electron E2E                                             |
| `three-stdlib` controls (evaluate)                    | custom `CameraController`                                | Only if it covers required interactions; otherwise keep custom            |

---

## 10. Note on the parallel `titapp` rewrite

A side-by-side rewrite (`source/titapp` + `titback`, branch `gui-fable`)
already implements several of these directions (case-owned projects, a
binary transport for bulk data, a single renderer state library). If that
rewrite remains the strategic path, the P0 correctness items here are still
worth fixing on `titgui` while it ships, but the P1/P2 structural work should
be invested in only one of the two codebases — doing both would be duplicate
effort. Decide which tree is the future before scheduling §4–§6.
