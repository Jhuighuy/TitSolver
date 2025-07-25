#!/usr/bin/env bash
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
# Commercial use, including SaaS, requires a separate license, see /LICENSE.md
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# This is a poor man's implementation of the moreutils's chronic.
# chronic is used to suppress output of a command in case it succeeds.

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Create temporary file to store the command output.
OUTPUT=$(mktemp)

# Run the command and record it's exit code.
"$@" >"$OUTPUT" 2>&1
EXIT_CODE=$?

# If exit code is not zero, print the output.
[ "$EXIT_CODE" -eq 0 ] || cat "$OUTPUT"

# Remove output file.
rm -f "$OUTPUT"

# Exit with exit code of the command.
exit "$EXIT_CODE"

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
