# `titgui`

Electron GUI for BlueTit Solver.

## Architecture

One npm package, four runtime contexts, strictly layered:

| Directory   | Context       | Contents                                                                                                                                                                                      |
| ----------- | ------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `bindings/` | main (native) | N-API addon (`titgui-bindings.node`, built by CMake) wrapping `tit::data` storage and the `titwcsph_case` specification; `index.ts` is the typed loader                                       |
| `main/`     | main          | Application lifecycle, `CaseManager` (open case, recents, revision-guarded edits), case-scoped `SessionManager` (storage, solver process, telemetry), windows, help protocol, persisted state |
| `preload/`  | preload       | Derives the `window.titgui` client from the IPC contract; validates results and events                                                                                                        |
| `renderer/` | renderer      | React 19 app: `common/` (design-system components, Three.js visual layer), `main/` (workspace, viewport, panes), `help/` (manual browser)                                                     |
| `shared/`   | all           | The IPC contract (`shared/ipc/`), zod schemas and domain types (`case.ts`, `storage.ts`, …), pure utilities                                                                                   |

Key invariants:

- **Everything crossing the main/renderer boundary is declared in
  `shared/ipc/contract.ts`** — channel names, argument and result schemas,
  the preload client, and main-process handler types are all derived from it.
- **Renderer state lives in jotai atoms** (`renderer/main/state/`);
  per-viewport state is a `createViewportState()` bundle, one per viewport
  tab. React Query is used only for window-persisted values.
- **The case document flows one way**: edits patch the _authored_ tree and
  are submitted with a revision; the main process materializes (fills
  defaults, coerces, validates) in C++ and pushes the document back. The
  renderer never fills defaults itself.
- **`assert` is for programmer invariants only**; conditions caused by data,
  files, or processes use `ensure` and surface as toasts + log entries.

## Commands

```sh
npm run dev        # start the app (Vite dev server + Electron)
npm run build      # package the production app (electron-forge)
npm run lint       # tsc + oxlint + oxfmt (zero-warning policy)
npm run format     # oxfmt
npm run test       # vitest unit suite
npm run coverage   # vitest with lcov coverage (coverage/lcov.info)
npm run e2e        # Playwright smoke test; requires `npm run build` first
npm run upgrade    # bump all dependencies (see policy below)
```

Both test suites are registered with CTest (`titgui/unit`, `titgui/e2e`), so
`./build/build.sh --test` covers the GUI together with the C++ suites. The
`Coverage` configuration produces `coverage/lcov.info`, which CI uploads to
Codecov and Sonar.

Development helpers (environment variables): `TITGUI_CASE=<dir>` opens a
case at startup, `TITGUI_USER_DATA=<dir>` redirects all persisted app state
(used by the E2E suite for isolation), `VITE_AUTO_RUN=1` starts the solver
right away, `TIT_ROOT=<dir>` overrides the installation root.

## Dependency policy

The stack deliberately rides close to the edge (React 19 with the React
Compiler, Electron 43, Vite 8, Tailwind 4, TypeScript 7 with the native
compiler, Base UI 1.x) — acceptable for a pre-release product, revisit
before any public release. Ground rules:

- Upgrades go through `npm run upgrade`, as a dedicated change, not mixed
  into feature work.
- After an upgrade, the gate is: `npm run lint`, `npm run test`, a packaged
  `npm run build`, `npm run e2e`, and a manual `npm run dev` smoke check of
  the viewport (WebGL and native-addon regressions do not always show up in
  tests).
- Pinned exceptions (no `^`) are deliberate: `electron`, `tailwindcss`, and
  `babel-plugin-react-compiler` — bump those consciously.
