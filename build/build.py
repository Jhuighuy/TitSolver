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

if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Build StormSPH.",
    )
    parser.add_argument(
        "-cfg",
        "--configuration",
        metavar="CONFIG",
        action="store",
        default="Release",
        choices=["Debug", "Release", "Coverage"],
        help="build configuration",
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
        "-cxx",
        "--cxx-compiler",
        metavar="EXE",
        action="store",
        default=None,
        help="C++ compiler executable",
    )
    parser.add_argument(
        "-vcpkg",
        "--vcpkg-root",
        metavar="PATH",
        action="store",
        default=None,
        help="vcpkg installation root path",
    )
    parser.add_argument(
        "-args",
        "--arguments",
        metavar="ARGS",
        action="store",
        nargs=argparse.REMAINDER,
        default=None,
        help="extra CMake arguments",
    )
    args = parser.parse_args()

    # Prepare the CMake arguments for configuring.
    cmake_args = ["cmake"]

    # Setup the source and build directories.
    source_dir = "src"
    cmake_output_dir = os.path.join("output", "cmake_output")
    cmake_args += ["-S", source_dir, "-B", cmake_output_dir]

    # Setup C++ compiler.
    cxx_compiler = args.cxx_compiler or os.environ.get("CXX")
    if args.cxx_compiler is not None:
        print("Using C++ compiler:", cxx_compiler)
        cmake_args.append(f"-DCMAKE_CXX_COMPILER={cxx_compiler}")

    # Setup the build configuration.
    configuration = args.configuration
    print("Configuration:", configuration)
    cmake_args.append(f"-DCMAKE_BUILD_TYPE={configuration}")

    # Setup the vcpkg root.
    vcpkg_root = (
        args.vcpkg_root
        or os.environ.get("VCPKG_ROOT")
        or os.environ.get("VCPKG_INSTALLATION_ROOT")
    )
    if not vcpkg_root:
        vcpkg_root_candidate = os.path.join(os.path.expanduser("~"), "vcpkg")
        if os.path.exists(vcpkg_root_candidate) and os.path.isdir(vcpkg_root_candidate):
            vcpkg_root = vcpkg_root_candidate
    if vcpkg_root:
        print("Using vcpkg root:", vcpkg_root)
        vcpkg_toolchain_file = os.path.join(
            vcpkg_root, "scripts", "buildsystems", "vcpkg.cmake"
        )
        cmake_args.append(f"-DCMAKE_TOOLCHAIN_FILE={vcpkg_toolchain_file}")

    # Append the extra arguments.
    if args.arguments:
        cmake_args += args.arguments

    # Run CMake!
    exit_code = subprocess.check_call(cmake_args)
    if exit_code != 0:
        sys.exit(exit_code)

    # Prepare the CMake arguments for building.
    cmake_args = ["cmake"]

    # Setup build directory.
    cmake_args += ["--build", cmake_output_dir]

    # Setup the build configuration.
    if configuration:
        cmake_args += ["--config", configuration]

    # Setup the number of threads.
    jobs = args.jobs
    if jobs:
        print(f"Building with {jobs} threads.")
        cmake_args += ["--", "-j", str(jobs)]

    # Run CMake!
    exit_code = subprocess.check_call(cmake_args)
    if exit_code != 0:
        sys.exit(exit_code)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
