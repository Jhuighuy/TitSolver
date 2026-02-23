#!/usr/bin/env bash
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of BlueTit Solver, under the MIT License.
# See /LICENSE.md for license information. SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Test Driver Script.
#
# The script orchestrates test execution. Though the Bash implementation might
# seem complex, it's significantly faster compared to the Python version.

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

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

print-usage() {
	echo "Usage: $0 [options...] -- <test-command>"
	echo ""
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
	echo "  -- <test-command>     Test command line arguments."
}

parse-args() {
	while [[ $# -gt 0 ]]; do
		case "$1" in
		# Options.
		--name)
			TEST_NAME="$2"
			shift 2
			;;
		--name=*)
			TEST_NAME="${1#*=}"
			shift 1
			;;
		--exit-code)
			EXIT_CODE="$2"
			shift 2
			;;
		--exit-code=*)
			EXIT_CODE="${1#*=}"
			shift 1
			;;
		--stdin)
			STDIN_PATH="$2"
			shift 2
			;;
		--stdin=*)
			STDIN_PATH="${1#*=}"
			shift 1
			;;
		--input-file)
			INPUT_PATHS+=("$2")
			shift 2
			;;
		--input-file=*)
			INPUT_PATHS+=("${1#*=}")
			shift 1
			;;
		--match-stdout)
			STDOUT_PATH="$2"
			shift 2
			;;
		--match-stdout=*)
			STDOUT_PATH="${1#*=}"
			shift 1
			;;
		--match-stderr)
			STDERR_PATH="$2"
			shift 2
			;;
		--match-stderr=*)
			STDERR_PATH="${1#*=}"
			shift 1
			;;
		--match-file)
			OUTPUT_PATHS+=("$2")
			shift 2
			;;
		--match-file=*)
			OUTPUT_PATHS+=("${1#*=}")
			shift 1
			;;
		--filter)
			SED_FILTERS+=("$2")
			shift 2
			;;
		--filter=*)
			SED_FILTERS+=("${1#*=}")
			shift 1
			;;
		--)
			TEST_COMMAND=("${@:2}")
			break
			;;
		# Help.
		-h | -help | --help)
			print-usage
			exit 0
			;;
		*)
			echo ""
			echo "Invalid argument: $1."
			echo ""
			print-usage
			exit 1
			;;
		esac
	done

	if [ -z "$TEST_NAME" ]; then
		echo ""
		echo "Test name must be specified."
		echo ""
		print-usage
		exit 1
	fi

	if [ -z "${TEST_COMMAND[*]}" ]; then
		echo ""
		echo "Test command must be specified."
		echo ""
		print-usage
		exit 1
	fi
}

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

setup-work-dir() {
	# Transform the test name into a directory name: append the output directory
	# and remove the tags. E.g. 'foo/bar[abc]' -> '<output-dir>/foo/bar'.
	WORK_DIR="$TEST_OUTPUT_DIR/${TEST_NAME%%\[*}"
	if ! mkdir -p "$WORK_DIR"; then
		echo "Failed to create the test directory '$WORK_DIR'."
		exit 1
	fi

	# Redirect the output to the file.
	exec 1>"$WORK_DIR/test.log" 2>&1
}

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

setup-input-file() {
	local FILE_PATH="$1"
	if [ ! -f "$FILE_PATH" ]; then
		echo "Failed to run: input file '$FILE_PATH' does not exist."
		exit 1
	fi
	local FILE="${2:-$(basename "$FILE_PATH")}"
	ln -s "$FILE_PATH" "$FILE"
}

setup-input() {
	echo "Setting up the test input..."
	if [ "$STDIN_PATH" ]; then
		setup-input-file "$STDIN_PATH" "stdin.txt"
	else
		touch "stdin.txt"
	fi
	for INPUT_PATH in "${INPUT_PATHS[@]}"; do
		setup-input-file "$INPUT_PATH"
	done
}

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

FILES_TO_MATCH=()

setup-output-file() {
	local FILE_PATH="$1"
	if [ ! -f "$FILE_PATH" ]; then
		echo "Failed to run: output file '$FILE_PATH' does not exist."
		exit 1
	fi
	local FILE="${2:-$(basename "$FILE_PATH")}"
	local EXPECTED_FILE="$FILE.expected"
	ln -s "$FILE_PATH" "$EXPECTED_FILE"
	FILES_TO_MATCH+=("$FILE")
}

setup-output() {
	echo "Setting up the test output..."
	[ "$STDOUT_PATH" ] && setup-output-file "$STDOUT_PATH" "stdout.txt"
	[ "$STDERR_PATH" ] && setup-output-file "$STDERR_PATH" "stderr.txt"
	for OUTPUT_PATH in "${OUTPUT_PATHS[@]}"; do
		setup-output-file "$OUTPUT_PATH"
	done
}

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

run-test() {
	echo "Running test..."
	echo ">>> ${TEST_COMMAND[*]}"
	"${TEST_COMMAND[@]}" <"stdin.txt" >"stdout.txt" 2>"stderr.txt"
}

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Prefer `gsed` to regular `sed`. This is essential on the BSD-like systems.
SED_EXE=$(command -v gsed || echo sed)

filter-file() {
	local FILE=$1
	local FILTERED_FILE=$2

	local FIRST=true
	for FILTER in "${SED_FILTERS[@]}"; do
		# Convert PCRE filter to sed.
		FILTER=${FILTER//\\d/[0-9]}
		FILTER=${FILTER//\\w/[A-Za-z_]}

		# Apply the filter.
		if [ "$FIRST" = true ]; then
			"$SED_EXE" -rE "$FILTER" "$FILE" >"$FILTERED_FILE"
			FIRST=false
		else
			"$SED_EXE" -rE "$FILTER" -i "$FILTERED_FILE"
		fi
	done
}

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

match-exit-code() {
	local ACTUAL_EXIT_CODE="$1"
	if (((EXIT_CODE & 255) != ACTUAL_EXIT_CODE)); then
		echo "Exit codes do not match!"
		echo "  Expected: $EXIT_CODE"
		echo "  Actual  : $ACTUAL_EXIT_CODE"
		return 1
	fi
}

match-file-checksum() {
	local FILE="$1"

	# Get the expected checksum.
	local EXPECTED_CHECKSUM_FILE="$FILE.checksum.expected"
	local EXPECTED_CHECKSUM
	EXPECTED_CHECKSUM=$(cat "$EXPECTED_CHECKSUM_FILE")

	# Get the actual checksum. P.S. Do we need to filter the file?
	local ACTUAL_CHECKSUM_FILE="$FILE.checksum.actual"
	local ACTUAL_CHECKSUM
	shasum "$FILE" | cut -d' ' -f1 >"$ACTUAL_CHECKSUM_FILE"
	ACTUAL_CHECKSUM=$(cat "$ACTUAL_CHECKSUM_FILE")

	# Match them.
	if [ "$ACTUAL_CHECKSUM" != "$EXPECTED_CHECKSUM" ]; then
		echo "Checksum of '$FILE' does not match!"
		echo "  Expected: $EXPECTED_CHECKSUM"
		echo "  Actual  : $ACTUAL_CHECKSUM"
		return 1
	fi
}

match-file-contents() {
	local FILE="$1"
	local EXPECTED_FILE="$FILE.expected"
	local ACTUAL_FILE="$FILE.actual"
	ln -s "$FILE" "$ACTUAL_FILE"

	# Filter the actual and expected files (in parallel).
	local FILTERED_EXPECTED_FILE="$EXPECTED_FILE.filtered"
	local FILTERED_ACTUAL_FILE="$ACTUAL_FILE.filtered"
	filter-file "$EXPECTED_FILE" "$FILTERED_EXPECTED_FILE"
	filter-file "$ACTUAL_FILE" "$FILTERED_ACTUAL_FILE"

	# Match them.
	local DIFF_FILE="$FILE.diff"
	diff --ignore-blank-lines --ignore-trailing-space \
		"$FILTERED_EXPECTED_FILE" "$FILTERED_ACTUAL_FILE" >"$DIFF_FILE"
	local DIFF_EXIT_CODE=$?
	if [ "$DIFF_EXIT_CODE" != 0 ]; then
		echo "File '$FILE' contents do not match!"
		echo "  Expected  : $WORK_DIR/$FILTERED_EXPECTED_FILE"
		echo "  Actual    : $WORK_DIR/$FILTERED_ACTUAL_FILE"
		echo "  Difference: $WORK_DIR/$DIFF_FILE"
		return 1
	fi
}

match-file() {
	local FILE=$1
	if [[ "$FILE" =~ \.checksum$ ]]; then
		match-file-checksum "${FILE%.checksum}"
	else
		match-file-contents "$FILE"
	fi
}

match() {
	echo "Matching results..."
	PASSED=true

	# Match exit code.
	match-exit-code "$1" || PASSED=false

	# Match the output.
	for FILE in "${FILES_TO_MATCH[@]}"; do
		match-file "$FILE" || PASSED=false
	done

	# Exit with status.
	if [ "$PASSED" = true ]; then
		echo "Test passed!"
	else
		echo "Test failed!"
		return 1
	fi
}

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

main() {
	parse-args "$@"

	setup-work-dir
	cd "$WORK_DIR" || exit $?

	setup-input
	setup-output
	run-test
	match $?
}

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

main "$@"
