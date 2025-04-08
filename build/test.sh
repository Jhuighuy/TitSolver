#!/usr/bin/env bash
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
# Commercial use, including SaaS, requires a separate license, see /LICENSE.md
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Test Runner Script.

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

source "$(dirname "$0")/build-utils.sh" || exit $?
JOBS="${JOBS:-$(get-num-cpus)}"
CTEST_EXE="${CTEST_EXE:-ctest}"

usage() {
  echo "Usage: $(basename "$0") [options]"
  echo ""
  echo "Options:"
  echo "  -h, --help       Print this help message."
  echo "  -j, --jobs <num> Number of threads to parallelize the build."
}

parse-args() {
  while [[ $# -gt 0 ]]; do
    case "$1" in
      # Options.
      -j | --jobs) JOBS="$2"; shift 2;;
      # Help.
      -h | -help | --help)             usage; exit 0;;
      *) echo "Invalid argument: $1."; usage; exit 1;;
    esac
  done
}

display-options() {
  echo "# Options:"
  [ "$JOBS" -gt 1      ] && echo "#   JOBS = $JOBS"
}

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

prepare-test-dir() {
  rm -rf "$TEST_OUTPUT_DIR"
  mkdir -p "$TEST_OUTPUT_DIR" || exit $?
}

run-tests() {
  echo "# Running tests with $JOBS threads..."

  # Setup the the test output directory.
  prepare-test-dir

  # Prepare the CTest arguments.
  local CTEST_ARGS
  CTEST_ARGS=("$CTEST_EXE" "--output-on-failure")

  # Parallelize the test execution.
  [ "$JOBS" -gt 1 ] && CTEST_ARGS+=("-j" "$JOBS")

  # Exclude long tests if the flag is not set.
  [ ! "$TIT_LONG_TESTS" ] && CTEST_ARGS+=("--exclude-regex" "\[long\]")

  # Run CTest.
  (cd "$TEST_DIR" && "${CTEST_ARGS[@]}") || exit $?
}

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Run the tests.
TIMEFORMAT="Done. Elapsed %R seconds."
time {
  echo-thick-separator
  echo "BlueTit Test Script"
  echo-thick-separator
  parse-args "$@"
  display-options
  run-tests
  echo-separator
}
echo-thick-separator

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
