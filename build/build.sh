#!/usr/bin/env bash
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
# Commercial use, including SaaS, requires a separate license, see /LICENSE.md
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Build Script.

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

DIRNAME=$(dirname "$0")
source "$DIRNAME/build-utils.sh" || exit $?
CONFIG="Release"
FORCE=false
RUN_TESTS=false
JOBS="${JOBS:-$(get-num-cpus)}"
CC=${CC:-gcc}
CXX=${CXX:-g++}
VCPKG_ROOT="${VCPKG_ROOT:-}"
CMAKE_EXE="${CMAKE_EXE:-cmake}"

usage() {
  echo "Usage: $(basename "$0") [options]"
  echo ""
  echo "Options:"
  echo "  -h, --help            Print this help message."
  echo "  -c, --config <config> Build configuration, default: 'Release'."
  echo "  -f, --force           Disable all static analysis during the build."
  echo "  -t, --test            Run tests after successfully building the project."
  echo "  -j, --jobs <num>      Number of threads to parallelize the build."
  echo ""
  echo "Advanced options:"
  echo "  --cc <path>           Override the default C compiler, default: 'gcc'."
  echo "                        Also available as environment variable 'CC'."
  echo "  --cxx <path>          Override the default C++ compiler, default: 'g++'."
  echo "                        Also available as environment variable 'CXX'."
  echo "  --vcpkg-root <path>   vcpkg package manager installation root path."
  echo "                        Also available as environment variable 'VCPKG_ROOT'."
  echo ""
  echo "Environment variables:"
  echo "  CC                    C compiler, default: 'gcc'."
  echo "  CXX                   C++ compiler, default: 'g++'."
  echo "  VCPKG_ROOT            vcpkg package manager installation root path."
}

parse-args() {
  while [[ $# -gt 0 ]]; do
    case "$1" in
    # Options.
    -c | --config)
      CONFIG="$2"
      shift 2
      ;;
    --config=*)
      CONFIG="${1#*=}"
      shift 1
      ;;
    -f | --force)
      FORCE=true
      shift
      ;;
    -t | --test)
      RUN_TESTS=true
      shift
      ;;
    -j | --jobs)
      JOBS="$2"
      shift 2
      ;;
    --jobs=*)
      JOBS="${1#*=}"
      shift 1
      ;;
    # Advanced options.
    --cc)
      CC="$2"
      shift 2
      ;;
    --cc=*)
      CC="${1#*=}"
      shift 1
      ;;
    --cxx)
      CXX="$2"
      shift 2
      ;;
    --cxx=*)
      CXX="${1#*=}"
      shift 1
      ;;
    --vcpkg-root)
      VCPKG_ROOT="$2"
      shift 2
      ;;
    --vcpkg-root=*)
      VCPKG_ROOT="${1#*=}"
      shift 1
      ;;
    # Help.
    -h | -help | --help)
      usage
      exit 0
      ;;
    *)
      echo "Invalid argument: $1."
      usage
      exit 1
      ;;
    esac
  done
}

display-options() {
  echo "# Options:"
  echo "#   CONFIG     = $CONFIG"
  echo "#   FORCE      = YES"
  echo "#   RUN_TESTS  = YES"
  echo "#   JOBS       = $JOBS"
  echo "#   CC         = $(which "$CC")"
  echo "#   CXX        = $(which "$CXX")"
  echo "#   VCPKG_ROOT = $VCPKG_ROOT"
}

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

prepare-build-dir() {
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

  # Do not print "Up-to-date" messages during installation. Weirdly enough, this
  # should be set up here, and not during the installation.
  CMAKE_ARGS+=("-D" "CMAKE_INSTALL_MESSAGE=LAZY")

  # Should we run static analysis?
  #
  # Note: We must add the configuration option on each invocation of CMake.
  # Otherwise, CMake will remember the previous value and won't update it.
  local SKIP_ANALYSIS
  SKIP_ANALYSIS=$([ "$FORCE" = true ] && echo "YES" || echo "NO")
  CMAKE_ARGS+=("-D" "SKIP_ANALYSIS=$SKIP_ANALYSIS")

  # Set the C/C++ compilers.
  # To override the system compilers, there are two options:
  #
  # 1. Specify them using CMake's `CMAKE_C_COMPILER` and `CMAKE_CXX_COMPILER`
  #    variables.
  #
  # 2. Export them as an environment variable `CC` and `CXX`.
  #
  # The former method appears more favorable for pure CMake. However, there's
  # an issue with this approach: the overridden compiler isn't recognized by
  # vcpkg. To ensure vcpkg uses our compiler of choice, we can create a
  # custom triplet following these steps:
  #
  # 1. Create `my-triplet.toolchain` with the following content:
  #
  #    set(CMAKE_C_COMPILER   "/path/to/compiler")
  #    set(CMAKE_CXX_COMPILER "/path/to/compiler++")
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
  export CC
  export CXX

  # Use ccache (if available).
  if command -v ccache &>/dev/null; then
    echo "# Found ccache, using it."
    CMAKE_ARGS+=("-D" "CMAKE_C_COMPILER_LAUNCHER=ccache")
    CMAKE_ARGS+=("-D" "CMAKE_CXX_COMPILER_LAUNCHER=ccache")
  fi

  # Find vcpkg.
  find-vcpkg
  local TOOLCHAIN_PATH="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
  if [ ! -f "$TOOLCHAIN_PATH" ]; then
    echo "# Unable to find vcpkg toolchain file! Check you installation."
    exit 1
  fi
  CMAKE_ARGS+=("-D" "CMAKE_TOOLCHAIN_FILE=$TOOLCHAIN_PATH")

  # Run CMake.
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

  # Run CMake.
  "${CMAKE_ARGS[@]}" || exit $?
}

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

install() {
  echo "# Installing..."

  local CMAKE_ARGS
  CMAKE_ARGS=("$CMAKE_EXE")
  CMAKE_ARGS+=("--install" "$OUTPUT_DIR")
  CMAKE_ARGS+=("--prefix" "$INSTALL_DIR")

  # Run CMake.
  "${CMAKE_ARGS[@]}" || exit $?
}

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Run the build.
TIMEFORMAT="Done. Elapsed %R seconds."
time {
  echo-thick-separator
  echo "BlueTit Build Script"
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
