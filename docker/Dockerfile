# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of the Tit Solver project, under the MIT License.
# See /LICENSE.md for license information. SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

FROM ubuntu:24.04

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

## Use bash as the default shell.
RUN ln -sf /bin/bash /bin/sh

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

## Install the packages.
RUN <<EOF
  # Install packages.
  apt-get update -y
  apt-get install -y                                                           \
    build-essential                                                            \
    clang-tidy-18                                                              \
    cmake                                                                      \
    curl                                                                       \
    git                                                                        \
    g++-14                                                                     \
    gnupg                                                                      \
    ninja-build                                                                \
    pkg-config                                                                 \
    python3                                                                    \
    python3-pip                                                                \
    zip

  # Clean up.
  apt-get -y clean
  rm -rf /var/lib/apt/lists/*
EOF

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

## Install the Python tools.
ENV PIP_BREAK_SYSTEM_PACKAGES=1
RUN pip3 install --no-cache-dir codespell gcovr

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

## Install vcpkg.
ENV VCPKG_ROOT=/root/vcpkg
ENV VCPKG_FORCE_SYSTEM_BINARIES=1
RUN git clone --depth=1 https://github.com/microsoft/vcpkg.git $VCPKG_ROOT
RUN $VCPKG_ROOT/bootstrap-vcpkg.sh && rm -rf $VCPKG_ROOT/CMakeFiles

# Install the manifested packages.
ENV VCPKG_DEFAULT_BINARY_CACHE=/root/vcpkg_cache
RUN mkdir -p $VCPKG_DEFAULT_BINARY_CACHE
WORKDIR /
COPY vcpkg.json .
RUN <<EOF
  # Install the manifested packages for each compiler.
  CXX_COMPILERS=(g++-14)
  for CXX in "${CXX_COMPILERS[@]}"; do
    export CXX
    $VCPKG_ROOT/vcpkg install
  done
  # Clean up.
  rm -rf $VCPKG_ROOT/buildtrees
  rm -rf ./vcpkg.json
EOF

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
