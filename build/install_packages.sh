#!/bin/sh
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
# Part of the Tit Solver project, under the MIT License
# See /LICENSE.md for license information.
# SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

BLUE_COLOR_CODE="\033[94m"
GREEN_COLOR_CODE="\033[92m"
YELLOW_COLOR_CODE="\033[93m"
RED_COLOR_CODE="\033[91m"
RESET_COLOR_CODE="\033[0m"

print_message() {
  echo $BLUE_COLOR_CODE "**" $* $RESET_COLOR_CODE
}

print_success() {
  echo $GREEN_COLOR_CODE "**" $* $RESET_COLOR_CODE
}

print_warning() {
  echo $YELLOW_COLOR_CODE "**" $* $RESET_COLOR_CODE
}

print_error() {
  echo $RED_COLOR_CODE "**" $* $RESET_COLOR_CODE >> 2
}

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

print_message "Installing packages..."

ensure() {
  echo $*
  $* || exit $?
}

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

install_pip_packages() {
  # Install gcovr.
  ensure pip3 install --user --upgrade gcovr
  # Install codespell.
  ensure pip3 install --user --upgrade codespell
}

print_message "Installing Python packages..."
install_pip_packages

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

install_apt_packages() {
  # Fetch fresh packages.
  ensure sudo apt -qq update
  # Install CMake.
  ensure sudo apt -qq install build-essential cmake
  # Install GCC 13.
  ensure sudo apt -qq install g++-13
  # Install LLVM 17 (clang + clang-tidy + OpenMP support).
  ensure wget https://apt.llvm.org/llvm.sh
  ensure chmod +x llvm.sh
  ensure sudo ./llvm.sh 17
  ensure sudo apt -qq install clang-17 clang-tidy-17 libomp-17-dev
  # Install IWYU.
  ensure sudo apt -qq install iwyu
  # Install Qt 6. It cannot be install with vcpkg.
  ensure sudo apt -qq install libglx-dev libgl1-mesa-dev qt6-base-dev
}

if command -v apt-get &> /dev/null; then
  print_message "Installing remaining packages with APT..."
  install_apt_packages
  print_success "Done."
  exit 0
fi

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

install_brew_packages() {
  # Fetch fresh packages, but do not upgrade them automatically.
  export HOMEBREW_NO_AUTO_UPDATE=1
  ensure brew update -q
  # Install CMake.
  ensure brew install -q cmake
  # Install GCC 13.
  ensure brew install -q gcc@13
  # Install LLVM 17.
  ensure brew install -q llvm@17
  export PATH="$(brew --prefix llvm@17)/bin:$PATH"
  # Install IWYU.
  ensure brew install -q include-what-you-use
  # Install Qt 6. It cannot be install with vcpkg.
  ensure brew install -q qt6
}

if command -v brew &> /dev/null; then
  print_message "Installing remaining packages with brew..."
  install_brew_packages
  print_success "Done."
  exit 0
fi

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
