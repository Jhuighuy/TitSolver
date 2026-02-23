#!/usr/bin/env bash
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of BlueTit Solver, under the MIT License.
# See /LICENSE.md for license information. SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Build Script.

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

SOURCE_DIR=$(cd "$(dirname "$0")/.." && pwd)
OUTPUT_DIR="$SOURCE_DIR/output"
BINARY_DIR="$OUTPUT_DIR/cmake_output"
INSTALL_DIR="$OUTPUT_DIR/TIT_ROOT"

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

RED=$'\e[0;31m'
GREEN=$'\e[0;32m'
BLUE=$'\e[0;36m'
BOLD=$'\e[1m'
RESET=$'\e[0m'
COLUMNS=$( (tty -s && tput cols) || echo -n "80")
TIMEFORMAT="${GREEN}Done${RESET}. Elapsed ${BOLD}%R${RESET} seconds."

print-separator() {
	local CHAR=${1:-"~"}
	for _ in $(seq 1 "$COLUMNS"); do echo -n "$CHAR"; done
	echo
}

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

CONFIG="Release"
FORCE=false
TESTS=false
LONG_TESTS=false
JOBS=$(getconf _NPROCESSORS_ONLN)
CC=${CC:-gcc}
CXX=${CXX:-g++}
VCPKG_ROOT="${VCPKG_ROOT:-$HOME/vcpkg}"

print-usage() {
	echo "${BLUE}${BOLD}Usage${RESET}: $0 [options...]"
	echo ""
	echo "${BLUE}Options${RESET}:"
	echo "  -h, --help          Print this help message."
	echo "  -c, --config <conf> Build configuration, default: '${BOLD}$CONFIG${RESET}'."
	echo "  -f, --force         Disable all static analysis during the build."
	echo "  -t, --test          Run tests after successfully building the project."
	echo "  -l, --long          Run long tests. Implies --test."
	echo "  -j, --jobs <num>    Number of jobs to parallelize the build, default: '${BOLD}$JOBS${RESET}'."
	echo ""
	echo "${BLUE}Advanced options${RESET}:"
	echo "  --cc <path>         Override the default C compiler, default: '${BOLD}$CC${RESET}'."
	echo "                      Also available as environment variable CC."
	echo "  --cxx <path>        Override the default C++ compiler, default: '${BOLD}$CXX${RESET}'."
	echo "                      Also available as environment variable CXX."
	echo "  --vcpkg-root <path> vcpkg installation root path, default: ${BOLD}$VCPKG_ROOT${RESET}."
	echo "                      Also available as environment variable VCPKG_ROOT."
	echo ""
	echo "${BLUE}Environment variables${RESET}:"
	echo "  CC                  C compiler."
	echo "  CXX                 C++ compiler."
	echo "  VCPKG_ROOT          vcpkg installation root path."
}

print-options() {
	echo "${BLUE}Options:${RESET}"
	echo "  FORCE      = ${BOLD}$FORCE${RESET}"
	echo "  TESTS      = ${BOLD}$TESTS${RESET}"
	echo "  LONG_TESTS = ${BOLD}$LONG_TESTS${RESET}"
	echo "  JOBS       = ${BOLD}$JOBS${RESET}"
	echo "  CC         = ${BOLD}$CC${RESET}"
	echo "  CXX        = ${BOLD}$CXX${RESET}"
	echo "  VCPKG_ROOT = ${BOLD}$VCPKG_ROOT${RESET}"
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
			TESTS=true
			shift
			;;
		-l | --long)
			TESTS=true
			LONG_TESTS=true
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
			print-usage
			exit 0
			;;
		*)
			echo ""
			echo "${RED}Unknown argument: $1.${RESET}"
			echo ""
			print-usage
			exit 1
			;;
		esac
	done
}

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

configure() {
	echo "${BLUE}${BOLD}Configuring...${RESET}"

	# Set the C/C++ compilers.
	export CC
	export CXX

	# Setup the build directory.
	BUILD_DIR="$BINARY_DIR/$CONFIG"
	if ! mkdir -p "$BUILD_DIR" >/dev/null 2>&1; then
		echo "${RED}Failed to create build directory: '$BUILD_DIR'.${RESET}"
		exit 1
	fi

	# Prepare arguments for CMake.
	local CMAKE_ARGS=("cmake")

	# Setup the source and build directories.
	CMAKE_ARGS+=("-S" "$SOURCE_DIR")
	CMAKE_ARGS+=("-B" "$BUILD_DIR")

	# Setup the build configuration.
	CMAKE_ARGS+=("-D" "CMAKE_BUILD_TYPE=$CONFIG")

	# Setup the vcpkg toolchain.
	if [ ! -f "$VCPKG_ROOT/.vcpkg-root" ]; then
		echo "${RED}Failed to locate vcpkg installation: '$VCPKG_ROOT'.${RESET}"
		echo "${RED}Please ensure the vcpkg root directory is set correctly.${RESET}"
		exit 1
	fi
	local VCPKG_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
	if [ ! -f "$VCPKG_TOOLCHAIN_FILE" ]; then
		echo "${RED}Failed to locate vcpkg toolchain file: '$VCPKG_TOOLCHAIN_FILE'.${RESET}"
		echo "${RED}Please ensure vcpkg is installed correctly and the toolchain file exists.${RESET}"
		exit 1
	fi
	CMAKE_ARGS+=("-D" "CMAKE_TOOLCHAIN_FILE=$VCPKG_TOOLCHAIN_FILE")
	CMAKE_ARGS+=("-D" "VCPKG_INSTALL_OPTIONS=--no-print-usage")

	# Toggle static analysis.
	CMAKE_ARGS+=("-D" "SKIP_ANALYSIS=$([ "$FORCE" = true ] && echo "YES" || echo "NO")")

	# Do not print "Up-to-date" messages during installation.
	CMAKE_ARGS+=("-D" "CMAKE_INSTALL_MESSAGE=LAZY")

	# Setup ccache to speed up the build.
	if command -v ccache &>/dev/null; then
		CMAKE_ARGS+=("-D" "CMAKE_C_COMPILER_LAUNCHER=ccache")
		CMAKE_ARGS+=("-D" "CMAKE_CXX_COMPILER_LAUNCHER=ccache")
	fi

	# Run CMake.
	"${CMAKE_ARGS[@]}" || exit $?
}

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

build() {
	echo "${BLUE}${BOLD}Building with $JOBS jobs...${RESET}"

	# Prepare the CMake arguments.
	local CMAKE_ARGS=("cmake")

	# Setup the build directory.
	CMAKE_ARGS+=("--build" "$BUILD_DIR")

	# Setup the build configuration.
	CMAKE_ARGS+=("--config" "$CONFIG")

	# Setup the number of parallel jobs.
	CMAKE_ARGS+=("--parallel" "$JOBS")

	# Run CMake.
	"${CMAKE_ARGS[@]}" || exit $?
}

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

install() {
	echo "${BLUE}${BOLD}Installing...${RESET}"

	# Prepare arguments for CMake.
	local CMAKE_ARGS=("cmake")

	# Setup the build directory.
	CMAKE_ARGS+=("--install" "$BUILD_DIR")

	# Setup the installation directory.
	CMAKE_ARGS+=("--prefix" "$INSTALL_DIR")

	# Run CMake.
	"${CMAKE_ARGS[@]}" || exit $?
}

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

test() {
	echo "${BLUE}${BOLD}Running tests with $JOBS jobs...${RESET}"

	# Setup the the test output directory.
	# This variable should be available for test driver script.
	export TEST_OUTPUT_DIR="$OUTPUT_DIR/test_output"
	if ! rm -rf "$TEST_OUTPUT_DIR" >/dev/null 2>&1; then
		echo "${RED}Failed to clean test output directory: '$TEST_OUTPUT_DIR'.${RESET}"
		exit 1
	fi

	# Print the test output directory for convenience.
	echo "${BLUE}${BOLD}Test output will be available in '$TEST_OUTPUT_DIR/'.${RESET}"

	# Setup the paths.
	export PATH="$INSTALL_DIR/bin:$INSTALL_DIR/private/bin:$PATH"

	# Prepare arguments for CTest.
	local CTEST_ARGS=("ctest")

	# Setup the number of parallel jobs.
	CTEST_ARGS+=("--parallel" "$JOBS")

	# Setup the test directory.
	CTEST_ARGS+=("--test-dir" "$BUILD_DIR/tests")

	# Toggle long tests.
	[ "$LONG_TESTS" = false ] && CTEST_ARGS+=("--exclude-regex" "\[long\]")

	# Run CTest.
	"${CTEST_ARGS[@]}"
	local EXIT_CODE=$?

	# Print the test output directory for convenience.
	echo "${BLUE}${BOLD}Test output is available in '$TEST_OUTPUT_DIR/'.${RESET}"

	# Exit with the test exit code on failure.
	[ "$EXIT_CODE" -ne 0 ] || exit "$EXIT_CODE"
}

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

main() {
	print-separator "="
	echo "${BLUE}${BOLD}BlueTit Build Script${RESET}"
	print-separator "="
	parse-args "$@"
	print-options
	print-separator

	time {
		configure
		print-separator
		build
		print-separator
		install
		print-separator
	}
	print-separator "="

	if [ "$TESTS" = true ]; then
		time {
			test
			print-separator
		}
		print-separator "="
	fi
}

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

main "$@"
