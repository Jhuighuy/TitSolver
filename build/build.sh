#!/usr/bin/env bash
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of the Tit Solver project, under the MIT License.
# See /LICENSE.md for license information. SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Build Script.

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

CONFIG="Release"
FORCE=false
RUN_TESTS=false
JOBS=1
COMPILER=$CXX
VCPKG_ROOT="${VCPKG_ROOT:-}"
NO_VCPKG=false
DRY=false
EXTRA_ARGS=()

usage() {
  echo "Usage: $(basename "$0") [options]"
  echo "Options:"
  echo "  -h, --help            Print this help message."
  echo "  -c, --config <config> Build configuration, default: Release."
  echo "  -f, --force           Disable all static analysis during the build."
  echo "  -t, --test            Run tests after successfully building the project."
  echo "  -j, --jobs <num>      Number of threads to parallelize the build."
  echo ""
  echo "Advanced options:"
  echo "  --compiler <path>     Override system's C++ compiler."
  echo "  --vcpkg-root <path>   Vcpkg package manager installation root path."
  echo "  --no-vcpkg            Do not use vcpkg for building the project."
  echo "  --dry                 Perform a dry build: discard all previously built data."
  echo "  -- <args>             Extra CMake configuration arguments."
  exit 1
}

parse_args() {
  while [[ $# -gt 0 ]]; do
    case "$1" in
      -h | -help | --help)
        usage;;
      -c | --config)
        CONFIG="$2"
        shift 2;;
      --config=*)
        CONFIG="${1#*=}"
        shift 1;;
      -f | --force)
        FORCE=true
        shift;;
      -t | --test)
        RUN_TESTS=true
        shift;;
      -j | --jobs)
        JOBS="$2"
        shift 2;;
      --compiler)
        COMPILER="$2"
        shift 2;;
      --compiler=*)
        COMPILER="${1#*=}"
        shift 1;;
      --vcpkg-root)
        VCPKG_ROOT="$2"
        shift 2;;
      --vcpkg-root=*)
        VCPKG_ROOT="${1#*=}"
        shift 2;;
      --no-vcpkg)
        NO_VCPKG=true
        shift;;
      --dry)
        DRY=true
        shift;;
      --)
        EXTRA_ARGS=("${@:2}")
        break;;
      *)
        echo "Invalid argument: $1."
        usage;;
    esac
  done
  # Display parsed options.
  echo "# Options:"
  [ "$CONFIG"          ] && echo "#   CONFIG:     $CONFIG"
  [ $FORCE = true      ] && echo "#   FORCE:      YES"
  [ $RUN_TESTS = true  ] && echo "#   RUN_TESTS:  YES"
  [ "$JOBS" -gt 1      ] && echo "#   JOBS:       $JOBS"
  [ "$COMPILER"        ] && echo "#   COMPILER:   $COMPILER"
  [ "$VCPKG_ROOT"      ] && echo "#   VCPKG_ROOT: $VCPKG_ROOT"
  [ $NO_VCPKG = true   ] && echo "#   NO_VCPKG:   YES"
  [ $DRY = true        ] && echo "#   DRY:        YES"
  [ "${EXTRA_ARGS[@]}" ] && echo "#   EXTRA_ARGS: ${EXTRA_ARGS[*]}"
}

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

CMAKE_EXE="cmake"

prepare_build_dir() {
  if [ "$DRY" = true ]; then
    echo "# Performing a dry build."
    rm -rf "$OUTPUT_DIR"
  fi
  mkdir -p "$OUTPUT_DIR"
}

find_vcpkg() {
  VCPKG_ROOT="${VCPKG_ROOT:-$VCPKG_INSTALLATION_ROOT}"
  VCPKG_ROOT="${VCPKG_ROOT:-$HOME/vcpkg}"
  if [ ! -d "$VCPKG_ROOT" ]; then
    echo "# Unable to find vcpkg!"
    exit 1
  fi
}

# Function to configure the project
configure() {
  local CMAKE_ARGS
  CMAKE_ARGS=("$CMAKE_EXE")
  CMAKE_ARGS+=("-S" "$SOURCE_DIR" "-B" "$OUTPUT_DIR")
  CMAKE_ARGS+=("-D" "CMAKE_BUILD_TYPE=$CONFIG")
  local SKIP_ANALYSIS
  if [ "$FORCE" = true ]; then
    echo "# 'Force' build: static analysis is disabled."
    SKIP_ANALYSIS="YES"
  else
    SKIP_ANALYSIS="NO"
  fi
  CMAKE_ARGS+=("-D" "SKIP_ANALYSIS=$SKIP_ANALYSIS")
  if [ -n "$COMPILER" ]; then
    echo "# Overriding system's C++ compiler: $COMPILER."
    # To override the system compiler, there are two available approaches:
    #
    # 1. Specify it using CMake's `CMAKE_CXX_COMPILER` variable.
    #
    # 2. Export it as an environment variable named `CXX`.
    #
    # The former method appears more favorable for pure CMake. However, there's
    # an issue with this approach: the overridden compiler isn't recognized by
    # vcpkg. To ensure vcpkg uses our designated compiler, we can create a
    # custom triplet following these steps:
    #
    # 1. Create `my-triplet.toolchain` with the following content:
    #
    #    set(CMAKE_CXX_COMPILER "my-favorite-compiler")
    #
    # 2. Create `my-triplet.cmake` with the following content:
    #
    #    set(VCPKG_CHAINLOAD_TOOLCHAIN_FILE "/path/to/my-triplet.toolchain")
    #    set(VCPKG_CRT_LINKAGE dynamic)
    #    set(VCPKG_LIBRARY_LINKAGE static)
    #
    # 3. Provide the following parameters to CMake:
    #
    #    -D VCPKG_OVERLAY_TRIPLETS=/path/to/triplet/and/toolchain/
    #    -D VCPKG_HOST_TRIPLET=my-triplet
    #    -D VCPKG_TARGET_TRIPLET=my-triplet
    #
    # In my view, using custom triplets solely to switch the compiler might be
    # excessive. Therefore, I'll stick to the environment variable method for
    # now.
    export CXX="$COMPILER"
  fi
  if [ "$NO_VCPKG" != true ]; then
    find_vcpkg
    TOOLCHAIN_PATH="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
    if [ ! -f "$TOOLCHAIN_PATH" ]; then
      echo "- Unable to find vcpkg toolchain file!"
      exit 1
    fi
    CMAKE_ARGS+=("-D" "CMAKE_TOOLCHAIN_FILE=$TOOLCHAIN_PATH")
  fi
  [ "${EXTRA_ARGS[@]}" ] && CMAKE_ARGS=("${CMAKE_ARGS[@]}" "${EXTRA_ARGS[@]}")
  "${CMAKE_ARGS[@]}" || exit $?
}

build() {
  local CMAKE_ARGS
  CMAKE_ARGS=("cmake")
  CMAKE_ARGS+=("--build" "$OUTPUT_DIR")
  CMAKE_ARGS+=("--config" "$CONFIG")
  if [ "$JOBS" -gt 1 ]; then
    echo "# Building with $JOBS threads."
    CMAKE_ARGS+=("-j" "$JOBS")
  fi
  "${CMAKE_ARGS[@]}" || exit $?
}

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

CTEST_EXE="ctest"

run_tests() {
  # Setup the the test output directory.
  rm -rf "$TEST_OUTPUT_DIR"
  mkdir -p "$TEST_OUTPUT_DIR"
  # Run CTest.
  local CTEST_ARGS
  CTEST_ARGS=("$CTEST_EXE" "--output-on-failure")
  # Output results summary to a JUnit XML file.
  CTEST_ARGS+=("--output-junit" "$TEST_OUTPUT_DIR/JUnit.xml")
  if [ "$JOBS" -gt 1 ]; then
    echo "# Running tests with $JOBS threads."
    CTEST_ARGS+=("-j" "$JOBS")
  fi
  if [ ! "$TIT_LONG_TESTS" ]; then
    # Exclude long tests.
    CTEST_ARGS+=("--exclude-regex" "\[long\]")
  fi
  (cd "$TEST_DIR" && "${CTEST_ARGS[@]}") || exit $?
}

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

START_TIME=$(date +%s.%N)
source "./build/build_utils.sh" || exit $?
echo_thick_banner
parse_args "$@"
echo_banner
echo "# Configuring..."
prepare_build_dir
configure
echo_banner
echo "# Building..."
build
if [ "$RUN_TESTS" = true ]; then
  echo_banner
  echo "# Running tests..."
  run_tests
fi
END_TIME=$(date +%s.%N)
ELAPSED=$(echo "$END_TIME - $START_TIME" | bc -l)
echo_thick_banner
printf "# Done. Elapsed: %ss.\n" "$ELAPSED"

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
