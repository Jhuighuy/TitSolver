# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of the Tit Solver project, under the MIT License.
# See /LICENSE.md for license information. SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

FROM ubuntu:24.04

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

## Install the packages.
RUN <<EOF
  # Install packages.
  apt-get update -y
  apt-get install -y                                                           \
    build-essential                                                            \
    clang-18                                                                   \
    clang-tidy-18                                                              \
    cmake                                                                      \
    curl                                                                       \
    git                                                                        \
    g++-13                                                                     \
    gnupg                                                                      \
    lsb-release                                                                \
    ninja-build                                                                \
    pkg-config                                                                 \
    python3                                                                    \
    python3-pip                                                                \
    software-properties-common                                                 \
    wget                                                                       \
    zip

  # Clean up.
  apt-get -y clean
  rm -rf /var/lib/apt/lists/*
EOF

## Use bash as the default shell.
RUN ln -sf /bin/bash /bin/sh

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

## Install the Python tools.
ENV PIP_BREAK_SYSTEM_PACKAGES=1
RUN pip3 install --no-cache-dir codespell gcovr

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

## Install vcpkg.
ENV CXX=g++-13
ENV VCPKG_ROOT=/root/vcpkg
ENV VCPKG_FORCE_SYSTEM_BINARIES=1
RUN <<EOF
  # Clone and bootstrap vcpkg.
  git clone --depth=1 https://github.com/microsoft/vcpkg.git $VCPKG_ROOT
  $VCPKG_ROOT/bootstrap-vcpkg.sh

  # Clean up.
  rm -rf $VCPKG_ROOT/CMakeFiles
EOF

# Install the manifested packages.
ENV VCPKG_DEFAULT_BINARY_CACHE=/root/vcpkg_cache
WORKDIR /
COPY vcpkg.json .
RUN <<EOF
  # Install the manifested packages.
  mkdir $VCPKG_DEFAULT_BINARY_CACHE
  $VCPKG_ROOT/vcpkg install

  # Clean up.
  rm -rf $VCPKG_ROOT/buildtrees
EOF

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
