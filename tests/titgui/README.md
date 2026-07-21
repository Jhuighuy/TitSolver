# `tests/titgui`

This directory registers the GUI test suites:

- `titgui/unit` — the vitest suite (`npm run test`); in the `Coverage`
  configuration it runs `npm run coverage` and produces
  `source/titgui/coverage/lcov.info`.
- `titgui/e2e` — the Playwright smoke test driving the real Electron app
  (`npm run e2e`). Requires a display: it runs as-is on macOS, and under
  `xvfb-run` on Linux (skipped when `xvfb-run` is not available).
