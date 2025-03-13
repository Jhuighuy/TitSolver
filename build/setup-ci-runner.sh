#!/usr/bin/env bash
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
# Commercial use, including SaaS, requires a separate license, see /LICENSE.md
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Setup CI environment.

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

GCC_VERSION=${GCC_VERSION:-14}
LLVM_VERSION=${LLVM_VERSION:-20}

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

enforce-ci() {
  [[ -n "$GITHUB_RUN_ID" ]] && return
  echo "This script must be run in a GitHub CI environment!"
  exit 1
}

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

setup-macos() {
  # Select the correct Xcode version.
  sudo xcode-select -s /Library/Developer/CommandLineTools
  # sudo xcode-select -s /Applications/Xcode_16.2.app

  # Install Homebrew packages.
  #
  # Upon installation, Homebrew will automatically update itself and upgrade
  # all installed packages that depend on those we are installing or upgrading.
  # The below variables will prevent Homebrew from doing this.
  export HOMEBREW_NO_AUTO_UPDATE=1
  export HOMEBREW_NO_INSTALLED_DEPENDENTS_CHECK=1
  brew update -q
  brew install -q        \
    automake             \
    cmake                \
    codespell            \
    diffutils            \
    "gcc@$GCC_VERSION"   \
    gnu-sed              \
    libtool              \
    "llvm@$LLVM_VERSION" \
    node                 \
    pkg-config           \
    pnpm                 \
    sphinx-doc           \
    || true # Linking may fail if the package is already installed.

  # Add LLVM tools to the path and create a versioned symlink for clang++.
  local LLVM_PATH
  LLVM_PATH="$(brew --prefix "llvm@$LLVM_VERSION")/bin"
  ln -s "$LLVM_PATH/clang++" "$LLVM_PATH/clang++-$LLVM_VERSION"
  echo "$LLVM_PATH" >> "$GITHUB_PATH"

  # Add Sphinx to the path.
  local SPHINX_PATH
  SPHINX_PATH="$(brew --prefix sphinx-doc)/bin"
  echo "$SPHINX_PATH" >> "$GITHUB_PATH"
}

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

setup-ubuntu() {
  # Install packages.
  sudo apt -qq update
  wget https://apt.llvm.org/llvm.sh
  chmod +x llvm.sh
  sudo ./llvm.sh "$LLVM_VERSION"
  sudo apt -qq install         \
    "clang-$LLVM_VERSION"      \
    "clang-tidy-$LLVM_VERSION" \
    cmake                      \
    "g++-$GCC_VERSION"         \
    nodejs                     \
    npm                        \
    sphinx-doc

  # Some packages are installed via pip.
  export PIP_BREAK_SYSTEM_PACKAGES=1
  pip3 install --upgrade --user \
    codespell                   \
    gcovr                       \
    sphinx

  # Some packages are installed via npm.
  sudo npm install -g pnpm
}

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

install-vcpkg() {
  export VCPKG_ROOT="$HOME/vcpkg"
  export VCPKG_DEFAULT_BINARY_CACHE="$HOME/vcpkg_cache"
  echo "VCPKG_ROOT=$VCPKG_ROOT" >> "$GITHUB_ENV"
  echo "VCPKG_DEFAULT_BINARY_CACHE=$VCPKG_DEFAULT_BINARY_CACHE" >> "$GITHUB_ENV"
  git clone https://github.com/microsoft/vcpkg.git "$VCPKG_ROOT" --depth=1
  "$VCPKG_ROOT/bootstrap-vcpkg.sh"
  mkdir -p "$VCPKG_DEFAULT_BINARY_CACHE" || true # Ignore if it already exists.

  # TODO: This is needed for some of our vcpkg packages to compile.
  #       Remove once all the packages are updated or older ones are dropped.
  #       Do not forget to update `build.sh` as well.
  echo "CMAKE_POLICY_VERSION_MINIMUM=3.5" >> "$GITHUB_ENV"
}

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

setup-ci() {
  # Install platform-specific dependencies.
  if [[ $OSTYPE == 'darwin'* ]]; then
    setup-macos
  elif [[ $OSTYPE == 'linux-gnu' ]]; then
    setup-ubuntu
  else
    echo "Unsupported OS: $OSTYPE!"
    exit 1
  fi

  # Install common dependencies.
  install-vcpkg
}

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

set -e # Exit on error.
enforce-ci
setup-ci

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
