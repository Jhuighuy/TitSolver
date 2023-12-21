#!/usr/bin/env bash
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
# Part of the Tit Solver project, under the MIT License.
# See /LICENSE.md for license information. SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

# Test Runner Script.
#
# The script orchestrates test execution. Though the Bash implementation might
# seem complex, it's significantly faster compared to the Python version.

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

TEST_NAME=""
TEST_COMMAND=()
EXIT_CODE=0
STDIN_PATH=""
STDOUT_PATH=""
STDERR_PATH=""
INPUT_PATHS=()
OUTPUT_PATHS=()
SED_FILTERS=(
  # Shrink absolute paths to filenames.
  "s/\/(([^\/]+|\.{1,2})\/)+([^\/]+)/\3/g"
  # Shrink source locations.
  "s/(\w+\.\w+):\d+:\d+/\1:<line>:<column>/g"
  # Remove escape sequences.
  "s/\x1B\[(\d+;)*\d+[mGK]//g"
  # Remove profiling reports.
  "/libgcov profiling error$/d"
)

usage() {
  echo "Usage: $(basename "$0") [options]"
  echo "Options:"
  echo "  -h, --help            Print this help message."
  echo "  --name <name>         Test name."
  echo "  --exit-code <code>    Expected test exit code."
  echo "  --stdin <path>        Provide as standard input for the test."
  echo "  --input-file <path>   Provide input file for the test."
  echo "  --match-stdout <path> Match test 'stdout' with the specified file."
  echo "  --match-stderr <path> Match test 'stderr' with the specified file."
  echo "  --match-file <path>   Match test output file with the specified file."
  echo "  --filter <filter>     Extra 'sed' filter to be applied to the test output."
  echo "  -- <args>             Test command line arguments."
  exit 1
}

parse_args() {
  while [[ $# -gt 0 ]]; do
    case "$1" in
      -h | -help | --help)
        usage;;
      --name)
        TEST_NAME="$2"
        shift 2;;
      --name=*)
        TEST_NAME="${1#*=}"
        shift 1;;
      --exit-code)
        EXIT_CODE="$2"
        shift 2;;
      --exit-code=*)
        EXIT_CODE="${1#*=}"
        shift 1;;
      --stdin)
        STDIN_PATH="$2"
        shift 2;;
      --stdin=*)
        STDIN_PATH="${1#*=}"
        shift 1;;
      --input-file)
        INPUT_PATHS+=("$2")
        shift 2;;
      --input-file=*)
        INPUT_PATHS+=("${1#*=}")
        shift 1;;
      --match-stdout)
        STDOUT_PATH="$2"
        shift 2;;
      --match-stdout=*)
        STDOUT_PATH="${1#*=}"
        shift 1;;
      --match-stderr)
        STDERR_PATH="$2"
        shift 2;;
      --match-stderr=*)
        STDERR_PATH="${1#*=}"
        shift 1;;
      --match-file)
        OUTPUT_PATHS+=("$2")
        shift 2;;
      --match-file=*)
        OUTPUT_PATHS+=("${1#*=}")
        shift 1;;
      --filter)
        SED_FILTERS+=("$2")
        shift 2;;
      --filter=*)
        SED_FILTERS+=("${1#*=}")
        shift 1;;
      --)
        TEST_COMMAND=("${@:2}")
        break;;
      *)
        echo "Invalid argument: $1."
        usage;;
    esac
  done
}

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

setup_work_dir() {
  echo "# Setting up the test directory..."
  # Transform the test name into a directory name: replace '::' with '/' and
  # remove the tag name. E.g. 'foo::bar[abc]' -> 'foo/bar'.
  WORK_DIR="$TEST_NAME"
  WORK_DIR="${WORK_DIR//:://}"
  WORK_DIR="${WORK_DIR%%\[*}"
  WORK_DIR="$TEST_OUTPUT_DIR/$WORK_DIR"
  mkdir -p "$WORK_DIR"
}

setup_input_file() {
  local FILE_PATH="$1"
  if [ ! -f "$FILE_PATH" ]; then
    echo "# Unable to run the test: input file $FILE_PATH does not exist!"
    exit 1
  fi
  local FILE="${2:-$(basename "$FILE_PATH")}"
  ln -s "$FILE_PATH" "$FILE"
}

setup_input() {
  echo "# Setting up the test input..."
  if [ "$STDIN_PATH" ]; then
    setup_input_file "$STDIN_PATH" "stdin.txt"
  else
    # Create the empty 'stdin' for convenience.
    touch "stdin.txt"
  fi
  for INPUT_PATH in "${INPUT_PATHS[@]}"; do
    setup_input_file "$INPUT_PATH"
  done
}

FILES_TO_MATCH=()

setup_output_file() {
  local FILE_PATH="$1"
  if [ ! -f "$FILE_PATH" ]; then
    echo "# Unable to run the test: output file $FILE_PATH does not exist!"
    exit 1
  fi
  local FILE="${2:-$(basename "$FILE_PATH")}"
  FILES_TO_MATCH+=("$FILE")
  local EXPECTED_FILE="$FILE.expected"
  ln -s "$FILE_PATH" "$EXPECTED_FILE"
}

setup_output() {
  echo "# Setting up the test output..."
  [ "$STDOUT_PATH" ] && setup_output_file "$STDOUT_PATH" "stdout.txt"
  [ "$STDERR_PATH" ] && setup_output_file "$STDERR_PATH" "stderr.txt"
  for OUTPUT_PATH in "${OUTPUT_PATHS[@]}"; do
    setup_output_file "$OUTPUT_PATH"
  done
}

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

run_test() {
  echo "# Running test..."
  echo "# $ ${TEST_COMMAND[*]}"
  "${TEST_COMMAND[@]}" <"stdin.txt" >"stdout.txt" 2>"stderr.txt"
}

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

match_exit_code() {
  local ACTUAL_EXIT_CODE="$1"
  if (( (EXIT_CODE & 255) != ACTUAL_EXIT_CODE )); then
    echo "# Exit codes do not match!"
    echo "# Note: expected $EXIT_CODE, actual $ACTUAL_EXIT_CODE."
    return 1
  fi
}

match_file_checksum() {
  local FILE="$1"
  # Get the expected checksum.
  local EXPECTED_CHECKSUM_FILE="$FILE.checksum.expected"
  local EXPECTED_CHECKSUM
  EXPECTED_CHECKSUM=$(cat "$EXPECTED_CHECKSUM_FILE")
  # Get the actual checksum. P.S. Do we need to filter the file?
  local ACTUAL_CHECKSUM_FILE="$FILE.checksum.actual"
  local ACTUAL_CHECKSUM
  shasum "$FILE" | cut -d' ' -f1 > "$ACTUAL_CHECKSUM_FILE"
  ACTUAL_CHECKSUM=$(cat "$ACTUAL_CHECKSUM_FILE")
  # Match them.
  if [ "$ACTUAL_CHECKSUM" != "$EXPECTED_CHECKSUM" ]; then
    echo "# Checksum of $FILE does not match!"
    echo "#   Expected: $EXPECTED_CHECKSUM"
    echo "#   Actual:   $ACTUAL_CHECKSUM"
    echo "#   Actual output: $WORK_DIR/$FILE"
    return 1
  fi
}

# Prefer `gsed` to regular `sed`. This is essential on the BSD-like systems.
SED_EXE=$(command -v gsed || echo sed)

filter_file() {
  local FILE=$1
  local FILTERED_FILE=$2
  local FIRST=true
  for FILTER in "${SED_FILTERS[@]}"; do
    # Convert PCRE filter to sed.
    FILTER=${FILTER//\\d/[0-9]}
    FILTER=${FILTER//\\w/[A-Za-z_]}
    # Apply the filter.
    if [ $FIRST = true ]; then
      $SED_EXE -rE "$FILTER" "$FILE" > "$FILTERED_FILE"
      FIRST=false
    else
      $SED_EXE -rE "$FILTER" -i "$FILTERED_FILE"
    fi
  done
}

match_file_contents() {
  local FILE="$1"
  local EXPECTED_FILE="$FILE.expected"
  local ACTUAL_FILE="$FILE.actual"
  ln -s "$FILE" "$ACTUAL_FILE"
  # Filter the actual and expected files (in parallel).
  local FILTERED_EXPECTED_FILE="$EXPECTED_FILE.filtered"
  local FILTERED_ACTUAL_FILE="$ACTUAL_FILE.filtered"
  filter_file "$EXPECTED_FILE" "$FILTERED_EXPECTED_FILE" &
  filter_file "$ACTUAL_FILE" "$FILTERED_ACTUAL_FILE"; wait
  # Match them.
  local DIFF_FILE="$FILE.diff"
  diff --ignore-blank-lines --ignore-trailing-space \
       "$FILTERED_EXPECTED_FILE" "$FILTERED_ACTUAL_FILE" >"$DIFF_FILE"
  local DIFF_EXIT_CODE=$?
  if [ $DIFF_EXIT_CODE != 0 ]; then
    echo "# File $FILE contents does not match!"
    echo "#   Difference: $WORK_DIR/$DIFF_FILE"
    echo "#   Actual output: $WORK_DIR/$FILE"
    return 1
  fi
}

match_file() {
  local FILE=$1
  # Determine the matching function.
  if [[ "$FILE" =~ \.checksum$ ]]; then
    FILE="${FILE%.checksum}"
    echo "# Matching $FILE checksum..."
    local MATCH_COMMAND=match_file_checksum
  else
    echo "# Matching $FILE contents..."
    local MATCH_COMMAND=match_file_contents
  fi
  # Ensure the file exists.
  if [ ! -f "$FILE" ]; then
    echo "# File $FILE does not exist!"
    return 1
  fi
  # Match the file.
  "$MATCH_COMMAND" "$FILE"
}

match() {
  echo "# Matching results..."
  PASSED=true
  # Match exit code.
  match_exit_code "$1" || PASSED=false
  # Match the output (in parallel).
  local PIDS=()
  for FILE in "${FILES_TO_MATCH[@]}"; do
    match_file "$FILE" & PIDS+=($!)
  done
  for PID in "${PIDS[@]}"; do wait "$PID" || PASSED=false; done
  # Exit with status.
  if [ "$PASSED" = true ]; then
    echo "# Test passed."
  else
    echo "# Test failed!"
    return 1
  fi
}

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

source "./build/build_utils.sh" || exit $?
echo_thin_banner
parse_args "$@"
setup_work_dir; cd "$WORK_DIR" || exit $?
setup_input
setup_output
run_test
match $?
STATUS=$?
echo_thin_banner
exit $STATUS

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
