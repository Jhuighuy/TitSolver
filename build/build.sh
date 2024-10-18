#!/usr/bin/env bash
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of the Tit Solver project, under the MIT License.
# See /LICENSE.md for license information. SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Build Script.

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

DIRNAME=$(dirname "$0")
source "$DIRNAME/build-utils.sh" || exit $?
CONFIG="Release"
FORCE=false
RUN_TESTS=false
JOBS=$(($(get-num-cpus) + 1))
COMPILER=$CXX
VCPKG_ROOT="${VCPKG_ROOT:-}"
DRY=false
EXTRA_ARGS=()
CMAKE_EXE="${CMAKE_EXE:-cmake}"

usage() {
  echo "Usage: $(basename "$0") [options] -- <cmake-args>"
  echo ""
  echo "Options:"
  echo "  -h, --help            Print this help message."
  echo "  -c, --config <config> Build configuration, default: Release."
  echo "  -f, --force           Disable all static analysis during the build."
  echo "  -t, --test            Run tests after successfully building the project."
  echo "  -j, --jobs <num>      Number of threads to parallelize the build."
  echo ""
  echo "Advanced options:"
  echo "  --compiler <path>     Override the default C++ compiler."
  echo "  --vcpkg-root <path>   Vcpkg package manager installation root path."
  echo "  --dry                 Perform a dry build: discard all previously built data."
  echo "  -- <cmake-args>       Additional CMake configuration arguments."
}

parse-args() {
  while [[ $# -gt 0 ]]; do
    case "$1" in
      # Options.
      -c | --config)  CONFIG="$2";           shift 2;;
      --config=*)     CONFIG="${1#*=}";      shift 1;;
      -f | --force)   FORCE=true;            shift;;
      -t | --test)    RUN_TESTS=true;        shift;;
      -j | --jobs)    JOBS="$2";             shift 2;;
      # Advanced options.
      --compiler)     COMPILER="$2";         shift 2;;
      --compiler=*)   COMPILER="${1#*=}";    shift 1;;
      --vcpkg-root)   VCPKG_ROOT="$2";       shift 2;;
      --vcpkg-root=*) VCPKG_ROOT="${1#*=}";  shift 2;;
      --dry)          DRY=true;              shift;;
      --)             EXTRA_ARGS=("${@:2}"); break;;
      # Help.
      -h | -help | --help)             usage; exit 0;;
      *) echo "Invalid argument: $1."; usage; exit 1;;
    esac
  done
}

display-options() {
  echo "# Options:"
  [ "$CONFIG"          ] && echo "#   CONFIG     = $CONFIG"
  [ $FORCE = true      ] && echo "#   FORCE      = YES"
  [ $RUN_TESTS = true  ] && echo "#   RUN_TESTS  = YES"
  [ "$JOBS" -gt 1      ] && echo "#   JOBS       = $JOBS"
  [ "$COMPILER"        ] && echo "#   COMPILER   = $COMPILER"
  [ "$VCPKG_ROOT"      ] && echo "#   VCPKG_ROOT = $VCPKG_ROOT"
  [ $DRY = true        ] && echo "#   DRY        = YES"
  [ "${EXTRA_ARGS[@]}" ] && echo "#   EXTRA_ARGS = ${EXTRA_ARGS[*]}"
}

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

prepare-build-dir() {
  if [ "$DRY" = true ]; then
    echo "# Discarding previous build."
    rm -rf "$OUTPUT_DIR"
  fi
  mkdir -p "$OUTPUT_DIR" || exit $?
}

find-vcpkg() {
  VCPKG_ROOT_CANDIDATES=(
    "$VCPKG_ROOT"
    "$VCPKG_INSTALLATION_ROOT"
    # Add custom paths here.
    "$HOME/vcpkg"
  )
  for VCPKG_ROOT in "${VCPKG_ROOT_CANDIDATES[@]}"; do
    if [ -f "$VCPKG_ROOT/.vcpkg-root" ]; then
      echo "# Found vcpkg at $VCPKG_ROOT."
      return
    fi
  done
  echo "# Unable to find vcpkg!"
  exit 1
}

configure() {
  echo "# Configuring..."

  # Setup the build directory.
  prepare-build-dir

  # Prepare the CMake arguments.
  local CMAKE_ARGS
  CMAKE_ARGS=("$CMAKE_EXE")
  CMAKE_ARGS+=("-S" "$SOURCE_DIR" "-B" "$OUTPUT_DIR")
  CMAKE_ARGS+=("-D" "CMAKE_BUILD_TYPE=$CONFIG")

  # Should we run static analysis?
  #
  # Note: We must add the configuration option on each invocation of CMake.
  # Otherwise, CMake will remember the previous value and won't update it.
  if [ "$FORCE" = true ]; then
    CMAKE_ARGS+=("-D" "SKIP_ANALYSIS=YES")
  else
    CMAKE_ARGS+=("-D" "SKIP_ANALYSIS=NO")
  fi

  # Set the C++ compiler.
  if [ -n "$COMPILER" ]; then
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

  # Find vcpkg.
  find-vcpkg
  local TOOLCHAIN_PATH="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
  if [ ! -f "$TOOLCHAIN_PATH" ]; then
    echo "# Unable to find vcpkg toolchain file! Check you installation."
    exit 1
  fi
  CMAKE_ARGS+=("-D" "CMAKE_TOOLCHAIN_FILE=$TOOLCHAIN_PATH")

  # Execute cmake.
  [ "${EXTRA_ARGS[@]}" ] && CMAKE_ARGS=("${CMAKE_ARGS[@]}" "${EXTRA_ARGS[@]}")
  "${CMAKE_ARGS[@]}" || exit $?
}

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

build() {
  echo "# Building with $JOBS threads..."

  # Prepare the CMake arguments.
  local CMAKE_ARGS
  CMAKE_ARGS=("$CMAKE_EXE")
  CMAKE_ARGS+=("--build" "$OUTPUT_DIR")
  CMAKE_ARGS+=("--config" "$CONFIG")

  # Parallelize the build.
  [ "$JOBS" -gt 1 ] && CMAKE_ARGS+=("-j" "$JOBS")

  # Execute cmake.
  "${CMAKE_ARGS[@]}" || exit $?
}

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

install() {
  echo "# Installing..."

  local CMAKE_ARGS
  CMAKE_ARGS=("$CMAKE_EXE")
  CMAKE_ARGS+=("--install" "$OUTPUT_DIR")
  CMAKE_ARGS+=("--prefix" "$INSTALL_DIR")

  "${CMAKE_ARGS[@]}" || exit $?
}

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Run the build.
TIMEFORMAT="Done. Elapsed %R seconds."
time {
  echo-thick-separator
  echo "Tit Build Script"
  echo-thick-separator
  parse-args "$@"
  display-options
  echo-separator
  configure
  echo-separator
  build
  install
  echo-separator
}

# Run the tests if the flag is set.
if [ "$RUN_TESTS" = true ]; then
  "$DIRNAME/test.sh" -j "$JOBS" || exit $?
else
  echo-thick-separator
fi

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
