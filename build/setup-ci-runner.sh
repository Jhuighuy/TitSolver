#!/usr/bin/env bash
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of the Tit Solver project, under the MIT License.
# See /LICENSE.md for license information. SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Setup CI environment.

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

GCC_VERSION=${GCC_VERSION:-14}
LLVM_VERSION=${LLVM_VERSION:-18}

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

enforce-ci() {
  [[ ! -z "$GITHUB_RUN_ID" ]] && return
  echo "This script must be run in a GitHub CI environment!"
  exit 1
}

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

setup-macos() {
  # Upon installation, Homebrew will automatically update itself and upgrade
  # all installed packages that depend on those we are installing or upgrading.
  # The below variables will prevent Homebrew from doing this.
  export HOMEBREW_NO_AUTO_UPDATE=1
  export HOMEBREW_NO_INSTALLED_DEPENDENTS_CHECK=1
  brew update -q
  brew install -q        \
    automake             \
    cmake                \
    diffutils            \
    "gcc@$GCC_VERSION"   \
    gnu-sed              \
    libtool              \
    "llvm@$LLVM_VERSION" \
    pkg-config
  local LLVM_PATH="$(brew --prefix llvm@$LLVM_VERSION)/bin"
  ln -s "$LLVM_PATH/clang++" "$LLVM_PATH/clang++-$LLVM_VERSION"
  echo "$LLVM_PATH" >> $GITHUB_PATH
}

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

setup-ubuntu() {
  sudo apt -qq update
  wget https://apt.llvm.org/llvm.sh
  chmod +x llvm.sh
  sudo ./llvm.sh $LLVM_VERSION
  sudo apt -qq install         \
    "clang-$LLVM_VERSION"      \
    "clang-tidy-$LLVM_VERSION" \
    cmake                      \
    "g++-$GCC_VERSION"
}

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

install-python-tools() {
  export PIP_BREAK_SYSTEM_PACKAGES=1
  pip3 install --upgrade --user \
    codespell                   \
    gcovr
}

install-vcpkg() {
  export VCPKG_ROOT="$HOME/vcpkg"
  export VCPKG_DEFAULT_BINARY_CACHE="$HOME/vcpkg_cache"
  echo "VCPKG_ROOT=$VCPKG_ROOT" >> $GITHUB_ENV
  echo "VCPKG_DEFAULT_BINARY_CACHE=$VCPKG_DEFAULT_BINARY_CACHE" >> $GITHUB_ENV
  # Note: we cannot use `--depth=1` here, vcpgk requires the baseline commit.
  git clone https://github.com/microsoft/vcpkg.git $VCPKG_ROOT
  "$VCPKG_ROOT/bootstrap-vcpkg.sh"
  mkdir -p $VCPKG_DEFAULT_BINARY_CACHE || true # Ignore if it already exists.
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
  install-python-tools
  install-vcpkg
}

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

set -e # Exit on error.
enforce-ci
setup-ci

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
