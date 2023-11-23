#!/usr/bin/env python3
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
# Part of the Tit Solver project, under the MIT License
# See /LICENSE.md for license information.
# SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

import argparse
import os
import shutil
import subprocess
import sys

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

BLUE_COLOR_CODE = "\033[94m"
GREEN_COLOR_CODE = "\033[92m"
YELLOW_COLOR_CODE = "\033[93m"
RED_COLOR_CODE = "\033[91m"
RESET_COLOR_CODE = "\033[0m"


def print_message(*args, **kwargs):
    """Print message."""
    print(BLUE_COLOR_CODE + "**", *args, RESET_COLOR_CODE, **kwargs)


def print_success(*args, **kwargs):
    """Print success message."""
    print(GREEN_COLOR_CODE + "**", *args, RESET_COLOR_CODE, **kwargs)


def print_warning(*args, **kwargs):
    """Print warning."""
    print(YELLOW_COLOR_CODE + "**", *args, RESET_COLOR_CODE, **kwargs)


def print_error(*args, **kwargs):
    """Print error."""
    print(RED_COLOR_CODE + "**", *args, RESET_COLOR_CODE, **kwargs)


# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

HOME_DIR = os.path.expanduser("~")
SOURCE_DIR = os.path.abspath(".")
OUTPUT_DIR = os.path.join(SOURCE_DIR, "output", "cmake_output")


def build():
    """Build Tit."""

    # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

    # Parse command line arguments.
    # (Do not start help for parameters with capital letter to match `-h`.)
    parser = argparse.ArgumentParser(
        description="Build Tit.",
    )
    parser.add_argument(
        "-c",
        "--config",
        metavar="<config>",
        action="store",
        default="Release",
        choices=["Debug", "Release", "Coverage"],
        help="build configuration",
    )
    parser.add_argument(
        "-f",
        "--force",
        action="store_true",
        help="disable all static analysis during the build (for experimenting)",
    )
    parser.add_argument(
        "-t",
        "--test",
        action="store_true",
        help="run tests after successfully building the project",
    )
    parser.add_argument(
        "-j",
        "--jobs",
        metavar="N",
        action="store",
        nargs="?",
        type=int,
        const=0,
        default=None,
        help="number of threads to parallelize the build",
    )
    parser.add_argument(
        "--compiler",
        metavar="EXE",
        action="store",
        default=None,
        help="override system's C++ compiler",
    )
    parser.add_argument(
        "--vcpkg-root",
        metavar="PATH",
        action="store",
        default=None,
        help="vcpkg package manager installation root path",
    )
    parser.add_argument(
        "--no-vcpkg",
        action="store_true",
        help="do not use vcpkg for building the project",
    )
    parser.add_argument(
        "--",
        dest="extra_args",
        metavar="<args>",
        action="store",
        nargs=argparse.REMAINDER,
        default=None,
        help="extra CMake arguments",
    )
    parser.add_argument(
        "--dry",
        action="store_true",
        help="perform a dry build: discard all previously built data.",
    )
    args = parser.parse_args()

    # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

    # Dry build?
    if args.dry:
        print_warning("Performing a dry build.")
        shutil.rmtree(OUTPUT_DIR, ignore_errors=True)

    # Create output directory.
    try:
        os.mkdir(OUTPUT_DIR)
    except FileExistsError:
        pass

    # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

    # Prepare the CMake arguments for configuring.
    cmake_args = ["cmake"]

    # Setup the source and build directories.
    cmake_args += ["-S", SOURCE_DIR, "-B", OUTPUT_DIR]

    # Setup the build configuration.
    config = args.config
    print_success(f"Configuration: {config}.")
    cmake_args.append("-D" + f"CMAKE_BUILD_TYPE={config}")

    # Setup static analysis.
    # (Should be always passed, since `-D...` options are cached).
    if args.force:
        print_warning("'Force' build: static analysis is disabled.")
    cmake_args.append("-D" + f"SKIP_ANALYSIS={'YES' if args.force else 'NO'}")

    # Setup vcpkg.
    if args.no_vcpkg:
        print_message("Building without vcpkg.")
        vcpkg_root = None
    else:
        vcpkg_root = (
            args.vcpkg_root
            or os.environ.get("VCPKG_ROOT")
            or os.environ.get("VCPKG_INSTALLATION_ROOT")
        )
        if not vcpkg_root:
            maybe_vcpkg_root = os.path.join(HOME_DIR, "vcpkg")
            if os.path.exists(maybe_vcpkg_root) and os.path.isdir(maybe_vcpkg_root):
                vcpkg_root = maybe_vcpkg_root
        if vcpkg_root:
            print_success(f"Using vcpkg root: {vcpkg_root}.")
            vcpkg_toolchain_file = os.path.join(
                vcpkg_root, "scripts", "buildsystems", "vcpkg.cmake"
            )
            cmake_args.append("-D" + f"CMAKE_TOOLCHAIN_FILE={vcpkg_toolchain_file}")
        else:
            print_error("Unable to find vcpkg.")
            sys.exit(-1)

    # Setup C++ compiler.
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
    # excessive. Therefore, I'll stick to the environment variable method for now.
    if compiler := (args.compiler or os.environ.get("CXX")):
        print_warning(f"Overriding system's C++ compiler: {compiler}.")
    if compiler := args.compiler:
        os.environ["CXX"] = compiler

    # Append the extra arguments.
    if args.extra_args:
        cmake_args += args.extra_args

    # Run CMake!
    if (exit_code := subprocess.call(cmake_args)) != 0:
        sys.exit(exit_code)

    # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

    # Prepare the CMake arguments for building.
    cmake_args = ["cmake"]

    # Setup build directory.
    cmake_args += ["--build", OUTPUT_DIR]

    # Setup the build configuration.
    if config:
        cmake_args += ["--config", config]

    # Setup the number of threads.
    jobs = args.jobs
    if jobs:
        print(f"Building with {jobs} threads.")
        cmake_args += ["--", "-j", str(jobs)]

    # Run CMake!
    if (exit_code := subprocess.call(cmake_args)) != 0:
        sys.exit(exit_code)

    # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

    if args.test:
        # Prepare CTest arguments.
        ctest_args = ["ctest", "--output-on-failure"]

        # Setup the number of threads.
        if jobs:
            ctest_args += ["-j", str(jobs)]

        # Prepare CTest working directory.
        ctest_cwd = os.path.join(OUTPUT_DIR, "tests")

        # Run CTest!
        if (exit_code := subprocess.call(ctest_args, cwd=ctest_cwd)) != 0:
            sys.exit(exit_code)


# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #


if __name__ == "__main__":
    build()
