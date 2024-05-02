# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of the Tit Solver project, under the MIT License.
# See /LICENSE.md for license information. SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

FROM ubuntu:23.10

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

## Install the packages.
RUN <<EOF
  set -ex

  # Install packages.
  apt-get update -y
  apt-get install -y                                                           \
    build-essential                                                            \
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

  # Add LLVM.
  wget -q https://apt.llvm.org/llvm.sh
  chmod +x llvm.sh
  ./llvm.sh 18
  apt-get install -y clang-18 clang-tidy-18 libomp-18-dev

  # Clean up.
  apt-get -y clean
  rm ./llvm.sh
  rm -rf /var/lib/apt/lists/*
EOF

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

## Install the Python tools.
RUN pip3 install --no-cache-dir --break-system-packages codespell gcovr

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

## Install vcpkg.
ENV CXX=g++-13
ENV VCPKG_ROOT=/root/vcpkg
ENV VCPKG_FORCE_SYSTEM_BINARIES=1
RUN <<EOF
  set -ex

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
  set -ex

  # Install the manifested packages.
  mkdir $VCPKG_DEFAULT_BINARY_CACHE
  $VCPKG_ROOT/vcpkg install

  # Clean up.
  rm -rf $VCPKG_ROOT/buildtrees
EOF

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
