#!/usr/bin/env bash
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of BlueTit Solver, under the MIT License.
# See /LICENSE.md for license information. SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Regolden tests by updating the expected output.

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

SOURCE_DIR=$(cd "$(dirname "$0")/.." && pwd)
TEST_OUTPUT_DIR="$SOURCE_DIR/output/test_output"

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

DIFF_FILES=$(
	find "$TEST_OUTPUT_DIR" \
		-type f \
		-name "*.diff" \
		! -path "*/test_driver/*" \
		! -size 0
)

for DIFF_FILE in $DIFF_FILES; do
	FILE="${DIFF_FILE%.diff}"
	if [[ ! -f "$FILE" ]]; then
		echo "Warning: output file '$FILE' does not exist. Skipping."
		continue
	fi

	EXPECTED_FILE="$FILE.expected"
	if [[ ! -f "$EXPECTED_FILE" ]]; then
		echo "Warning: golden file '$EXPECTED_FILE' does not exist. Skipping."
		continue
	fi

	echo "Regolding '$FILE'..."
	cp -f "$FILE" "$EXPECTED_FILE"
done

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
