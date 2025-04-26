#!/usr/bin/env bash
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
# Commercial use, including SaaS, requires a separate license, see /LICENSE.md
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Setup CI environment.

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# This script must be run in a GitHub CI environment.
if [[ -z "$GITHUB_RUN_ID" ]]; then
  echo "This script must be run in a GitHub CI environment!"
  exit 1
fi

# Exit on error.
set -e

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Select the compiler from the arguments.
GCC_VERSION=${GCC_VERSION:-14}
LLVM_VERSION=${LLVM_VERSION:-20}
case "$1" in
  GCC)
    export CC="gcc-$GCC_VERSION"
    export CXX="g++-$GCC_VERSION"
    ;;
  Clang)
    export CC="clang-$LLVM_VERSION"
    export CXX="clang++-$LLVM_VERSION"
    ;;
  *)
    echo "Unknown compiler: '$1'. Must be either 'GCC' or 'Clang'."
    exit 1
    ;;
esac
echo "CC=$CC" >> "$GITHUB_ENV"
echo "CXX=$CXX" >> "$GITHUB_ENV"

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Install dependencies.
PACKAGES=(
  cmake
  codespell
  "gcc@$GCC_VERSION"
  gcovr
  "llvm@$LLVM_VERSION"
  node
  pnpm
  sphinx-doc
)
if [[ "$OSTYPE" == "darwin"* ]]; then
  PACKAGES+=(
    diffutils
    gnu-sed
  )
fi

# Upon installation, Homebrew will automatically update itself and upgrade all
# installed packages that depend on those we are installing or upgrading.
# The below variables will prevent Homebrew from doing this.
export HOMEBREW_NO_AUTO_UPDATE=1
export HOMEBREW_NO_INSTALLED_DEPENDENTS_CHECK=1
export HOMEBREW_NO_INSTALL_CLEANUP=1
brew update -q
brew install -q "${PACKAGES[@]}" || true # Continue on failure.

# Add LLVM tools to the path.
LLVM_PATH="$(brew --prefix "llvm@$LLVM_VERSION")/bin"
ln -s "$LLVM_PATH/clang++" "$LLVM_PATH/clang++-$LLVM_VERSION"
echo "$LLVM_PATH" >> "$GITHUB_PATH"

# Add Sphinx to the path.
SPHINX_PATH="$(brew --prefix sphinx-doc)/bin"
echo "$SPHINX_PATH" >> "$GITHUB_PATH"

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Install vcpkg.
export VCPKG_ROOT="$HOME/vcpkg"
export VCPKG_DEFAULT_BINARY_CACHE="$HOME/vcpkg_cache"
echo "VCPKG_ROOT=$VCPKG_ROOT" >> "$GITHUB_ENV"
echo "VCPKG_DEFAULT_BINARY_CACHE=$VCPKG_DEFAULT_BINARY_CACHE" >> "$GITHUB_ENV"
git clone https://github.com/microsoft/vcpkg.git "$VCPKG_ROOT" --depth=1
"$VCPKG_ROOT/bootstrap-vcpkg.sh"
mkdir -p "$VCPKG_DEFAULT_BINARY_CACHE" || true # Ignore if it already exists.

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
