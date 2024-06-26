#!/usr/bin/env bash
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of the Tit Solver project, under the MIT License.
# See /LICENSE.md for license information. SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Common paths to the various build-related locations.
SOURCE_DIR=$(pwd)
export SOURCE_DIR
export OUTPUT_DIR="$SOURCE_DIR/output/cmake_output"
export INSTALL_DIR="$SOURCE_DIR/output/TIT_ROOT"
export TEST_DIR="$OUTPUT_DIR/tests"
export TEST_OUTPUT_DIR="$SOURCE_DIR/output/test_output"

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# TODO: there is a problem in Github Actions that we an error message is
# printed:
#
# tput: No value for $TERM and no -T specified
#
# I cannot reproduce this on MacOS.
export COLUMNS=${COLUMNS:-$([ -n "$TERM" ] && tput cols || echo 80)}

# Print banner.
echo_banner() {
  CHAR=${1:-"~"}
  for _ in $(seq 1 "$COLUMNS"); do echo -n "$CHAR"; done
  echo
}

# Print thick banner.
echo_thick_banner() {
  echo_banner "="
}

# Print thin banner.
echo_thin_banner() {
  echo_banner "-"
}

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
