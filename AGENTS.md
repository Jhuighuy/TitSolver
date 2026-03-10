# BlueTit Solver – Agent Instructions

## Project Overview

BlueTit Solver is a multi-language CFD/SPH codebase centered on a C++26 solver
stack and built through a single top-level CMake project.

Primary areas:

- `source/tit/` – core C++ libraries:
  - `core` – low-level utilities, assertions, platform code, stats, profiler,
    SIMD/vector/matrix helpers
  - `data` – HDF5, SQLite, storage, schema/type helpers
  - `geom` – grids, partitions, searching, spatial sorting
  - `par` – TBB-based parallel utilities
  - `sph` – SPH kernels, equations, particle structures, integrators
  - `testing` – doctest-based test support
- `source/titwcsph/` – WCSPH executable
- `source/titesph/` – elastic/SPH executable
- `source/titback/` – backend executable using Crow + JSON
- `source/titgui/` – Electron + React + TypeScript GUI
- `source/titparaview/` – ParaView integration:
  - `ttdb/` – shared library plugin
  - `ttdbreader/` – Python reader script
- `manual/` – Sphinx documentation
- `tests/` – CTest registrations, behavioral tests, application tests, and test
  driver validation

## Working Rules

- Inspect the relevant module before editing; this repo mixes C++, Python,
  TypeScript, Electron, and Sphinx assets.
- Prefer minimal, local changes that fit the surrounding style.
- Do not edit generated or vendored output unless the user explicitly asks:
  - `output/`
  - `source/titgui/dist/`
  - `source/titgui/.vite/`
  - `source/titgui/node_modules/`
- When changing C++ source or headers, run `clang-format -i` on the changed
  files before finishing.
- For this repository, always use `./build/build.sh` for builds and tests.
- If a change touches only web code under `source/titgui/`, you may instead run
  the relevant frontend checks directly from `source/titgui/` (for example
  `npm run lint`, `npx tsc --noEmit`, or other package-local web build/test
  commands).
- Do not invoke CMake or CTest directly as the primary workflow.
- Do not run test executables directly. Run them through
  `./build/build.sh --run` so the test driver can provide the
  expected clean working directory, input files, and output matching.

## Build Commands

Standard build flow:

```bash
./build/build.sh --config Release
./build/build.sh --config Debug
./build/build.sh --config Coverage
```

Targeted builds:

```bash
./build/build.sh --config Release --target tit::core
./build/build.sh --config Release --target tit::sph
./build/build.sh --config Release --target tit::titwcsph
```

Tests:

```bash
./build/build.sh --config Release --test
./build/build.sh --config Release --run "tests/tit/core"
./build/build.sh --config Release --run "math" --run "vec"
./build/build.sh --config Release --long
```

Toolchain and environment notes:

- `VCPKG_ROOT` must point to a valid vcpkg checkout. The script defaults to
  `~/vcpkg`.
- The build script defaults to `gcc-15` / `g++-15` unless `CC` / `CXX` or
  `--cc` / `--cxx` are provided.
- Output is written under `output/`, primarily:
  - `output/cmake_output/<config>/`
  - `output/TIT_ROOT/`
  - `output/test_output/`
- `--force` disables static analysis during configuration.
- Coverage builds imply analysis is skipped and may emit gcovr reports via
  `CODECOV_REPORT` and `SONAR_REPORT`.

## Test Structure

There are four practical testing layers in this repo:

- Unit-test executables built from co-located `*.test.cpp` files in
  `source/tit/**`, for example:
  - `tit::core_tests`
  - `tit::data_tests`
  - `tit::geom_tests`
  - `tit::par_tests`
  - `tit::sph_tests`
  - `tit::testing_tests`
- Behavioral tests under `tests/tit/core/**` using `add_tit_test(...)` to
  validate assertions, exception handling, signal handling, profiler output, and
  stats output.
- Test-driver self-tests under `tests/test_driver/` that validate the custom
  test harness behavior.
- Application-level testing under `tests/titwcsph/`, currently including the
  long-running `dam_breaking[long]` case.

CTest names follow the directory naming convention, for example:

- `tests/tit/core/unit`
- `tests/tit/core/catch/exception/std`
- `tests/tit/core/profiler`
- `tests/test_driver/check_stdout`
- `tests/titwcsph/dam_breaking[long]`

## Build System Notes

The top-level project:

- reads the version from `vcpkg.json`
- adds `source/`, `manual/`, and `tests/`
- wires in custom CMake modules from `cmake/`
- installs third-party license files from the vcpkg tree

Common CMake helpers used in this repository:

- `add_tit_library(...)`
- `add_tit_executable(...)`
- `add_tit_test(...)`
- `add_tit_node_package(...)`
- `add_tit_sphinx_target(...)`

Targets generally use the `tit::` namespace. Library targets observed in-tree
include:

- `tit::malloc`
- `tit::core`
- `tit::data`
- `tit::geom`
- `tit::par`
- `tit::sph`
- `tit::testing`
- `tit::ttdb`

Executable targets observed in-tree include:

- `tit::core_tests`
- `tit::data_tests`
- `tit::geom_tests`
- `tit::par_tests`
- `tit::sph_tests`
- `tit::testing_tests`
- `tit::titback`
- `tit::titwcsph`
- `tit::titesph`

## Language-Specific Notes

### C++

- Standard: C++26 project-wide, with heavy use of concepts, ranges, CTAD, and.
- Headers use `#pragma once`.
- File header convention:

```cpp
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
```

- Includes are rooted at `source/`, e.g.:

```cpp
#include "tit/core/math.hpp"
```

Assertion/error macros commonly used:

```cpp
TIT_ASSERT(expr, "message")
TIT_ALWAYS_ASSERT(expr, "message")
TIT_THROW(ExceptionType, "message")
TIT_ENSURE(expr, "message")
```

### GUI (`source/titgui/`)

- Stack: Electron Forge, Vite, React 19, TypeScript, Three.js.
- Styling/tooling present: ESLint, Tailwind CSS v4, Radix UI Themes, Zod,
  React Query.
- Common npm scripts:

```bash
npm run dev
npm run build
npm run lint
```

- Prefer editing `src/`, config files, and assets. Avoid touching generated
  output in `.vite/` or `dist/`.

### Manual (`manual/`)

- Sphinx-based documentation with `index.rst`, `conf.py`, `_templates/`, and
  `_static/`.
- Built through CMake via `add_tit_sphinx_target(...)`, not by ad hoc output
  checked into the repo.

### ParaView (`source/titparaview/`)

- Mixed C++ and Python.
- `ttdb/` builds a shared library plugin and installs `ttdb.py`.
- `ttdbreader/` installs `TTDBReader.py`.

## Code Style

Formatting and linting:

- Formatter: `clang-format` using the repo `.clang-format`
- Linter: `clang-tidy` using the repo `.clang-tidy`
- Spell checker: `codespell`

Important `clang-format` choices in this repo:

- `BasedOnStyle: LLVM`
- `BinPackArguments: No`
- `BinPackParameters: OnePerLine`
- `AlwaysBreakTemplateDeclarations: Yes`
- `RequiresClausePosition: OwnLine`
- `PointerAlignment: Left`
- `QualifierAlignment: Left`

`clang-tidy` is strict:

- broad `bugprone-*`, `concurrency-*`, `cppcoreguidelines-*`, `misc-*`,
  `modernize-*`, `performance-*`, and `readability-*`
- `WarningsAsErrors: *`

## Naming Conventions

| Element             | Convention       | Example         |
| ------------------- | ---------------- | --------------- |
| Types/classes       | `PascalCase`     | `ParticleArray` |
| Functions/variables | `snake_case`     | `read_file()`   |
| Private members     | `snake_case_`    | `size_`         |
| Macros              | `TIT_UPPER_CASE` | `TIT_ASSERT`    |
| Namespaces          | `tit::subsystem` | `tit::geom`     |
| Headers             | `.hpp`           | `math.hpp`      |
| Sources             | `.cpp`           | `env.cpp`       |
| Tests               | `.test.cpp`      | `math.test.cpp` |

## Practical Guidance For Agents

- If the task is C++-only, stay within the relevant module under `source/tit/`
  and its matching `tests/tit/...` registration.
- If the task affects runtime behavior or CLI output, check whether there is a
  behavioral test under `tests/tit/core/` that should be updated.
- If the task affects the long-running solver application, inspect
  `tests/titwcsph/`.
- If the task touches GUI behavior, validate whether the change belongs in the
  Electron background process, preload layer, React components, or Three.js
  visualization code.
- If you add new files, follow the existing header/banner conventions for the
  relevant language and module.
- Prefer building the narrowest relevant target first, then run the narrowest
  relevant test pattern.
