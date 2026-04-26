# BlueTit Solver – Agent Instructions

## Project Overview

BlueTit Solver is a multi-language CFD/SPH codebase centered on a C++26 solver
stack and built through a single top-level CMake project.

Primary areas:

- `source/tit/` – core C++ libraries:
  - `core` – low-level utilities.
  - `data` – SQLite, storage, schema/type helpers.
  - `geom` – grids, partitions, searching, spatial sorting.
  - `par` – TBB-based parallel utilities.
  - `prop` – general-purpose property/config management.
  - `sph` – SPH kernels, equations, particle structures, integrators.
  - `testing` – doctest-based test support.
- `source/titwcsph/` – WCSPH solver executable.
- `source/titesph/` – elastic/SPH solver executable.
- `source/titgui/` – Electron + React + TypeScript GUI.
- `source/titparaview/` – ParaView integration.
- `manual/` – Sphinx documentation.
- `tests/` – CTest registrations, behavioral tests, application tests, and test
  driver validation.

## Working Rules

- This repository is a VERY EARLY WIP. Proposing sweeping changes that improve
  long-term maintainability is encouraged.
- Long term maintainability is a core priority. If you add new functionality,
  first check if there is shared logic that can be extracted to a separate
  module. Duplicate logic across multiple files is a code smell and should be
  avoided.
- Don't be afraid to change existing code.
- Don't take shortcuts by just adding local logic to solve a problem.
- Inspect the relevant module before editing.
- When in doubt, always check the surrounding code for context/style/patterns.
- Do not edit generated or vendored output.
- When changing source files format the changed files before finishing.
- Always use `./build/build.sh` for builds and tests.
  If a change touches only web code under `source/titgui/`, you may instead run
  the relevant frontend checks directly from `source/titgui/` (for example
  `npm run lint`, `npx tsc --noEmit`, or other package-local web build/test
  commands).
- Make sure that linters and static analyzers are happy before finishing.
- Do not invoke CMake or CTest directly as the primary workflow.
- Do not run test executables directly. Run them through
  `./build/build.sh --run` so the test driver can provide the
  expected clean working directory, input files, and output matching.

## Build Commands

- Standard build flow:

```bash
./build/build.sh --config Release
./build/build.sh --config Debug
./build/build.sh --config Coverage
```

- Targeted builds:

```bash
./build/build.sh --config Release --target core
./build/build.sh --config Release --target sph
./build/build.sh --config Release --target titwcsph
```

- Tests:

```bash
./build/build.sh --config Release --test
./build/build.sh --config Release --run "tit/core"
./build/build.sh --config Release --run "math" --run "vec"
./build/build.sh --config Release --long
```

- Output is written under `output/`, primarily:
  - `output/cmake_output/<config>/`
  - `output/TIT_ROOT/`
  - `output/test_output/`
- `--force` disables static analysis during configuration. Use only for
  troubleshooting.

## Test Structure

- Unit-test executables built from co-located `*.test.cpp` files in
  `source/tit/**`.
- Tests are registered under `tests/**` using `add_tit_test(...)`.
- CTest names follow the directory naming convention, for example:
  `tit/core/unit`, `tit/core/profiler`, `titwcsph/dam_breaking[long]`.

## Language-Specific Notes

### C++

- Standard: C++26 project-wide, with heavy use of concepts, ranges, CTAD, and.
- Assertion/error macros commonly used:

```cpp
TIT_ASSERT(expr, "message");
TIT_ALWAYS_ASSERT(expr, "message");
TIT_THROW("message");
TIT_ENSURE(expr, "message");
```

- Naming Conventions:

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

### GUI

- Stack: Electron Forge, Vite, React 19, TypeScript, Three.js.
- Styling/tooling present: ESLint, Tailwind CSS v4, Base UI, Zod, React Query.
- Common npm scripts:

```bash
npm run dev
npm run build
npm run lint
```
