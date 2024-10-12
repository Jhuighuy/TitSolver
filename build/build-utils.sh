#!/usr/bin/env bash
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of the Tit Solver project, under the MIT License.
# See /LICENSE.md for license information. SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Various build-related utilities.

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Check if we are in the project root directory.
SOURCE_DIR=$(pwd)
if [ ! -f "$SOURCE_DIR/build/build-utils.sh" ]; then
  echo "# Build script must be executed from the project root directory!"
  exit 1
fi

# Common paths to the various build-related locations.
export SOURCE_DIR
export OUTPUT_DIR="$SOURCE_DIR/output/cmake_output"
export INSTALL_DIR="$SOURCE_DIR/output/TIT_ROOT"
export TEST_DIR="$OUTPUT_DIR/tests"
export TEST_OUTPUT_DIR="$SOURCE_DIR/output/test_output"

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Terminal width.
export COLUMNS=$((tty -s && tput cols) || echo 80)

# Print a separator.
echo-separator() {
  CHAR=${1:-"~"}
  for _ in $(seq 1 "$COLUMNS"); do echo -n "$CHAR"; done
  echo
}

# Print thick banner.
echo-thick-separator() {
  echo-separator "="
}

# Print thin banner.
echo-thin-separator() {
  echo-separator "-"
}

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Print the number of logical CPUs.
get-num-cpus() {
  getconf _NPROCESSORS_ONLN
}

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
