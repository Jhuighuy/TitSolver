#!/usr/bin/env python3
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
# Part of the Tit Solver project, under the MIT License.
# See /LICENSE.md for license information. SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

import argparse
import os
import pathlib
import shutil
import subprocess
import sys

HOME_DIR = os.path.expanduser("~")
SOURCE_DIR = os.path.abspath(".")
OUTPUT_DIR = os.path.join(SOURCE_DIR, "output", "cmake_output")
TEST_DIR = os.path.join(OUTPUT_DIR, "tests")

CMAKE_EXE = "cmake"
CTEST_EXE = "ctest"


def parse_args():
    """Parse command line arguments."""
    parser = argparse.ArgumentParser(description=main.__doc__)
    parser.add_argument(
        "-c",
        "--config",
        metavar="<config>",
        help="build configuration",
        action="store",
        default="Release",
        choices=["Debug", "Release", "Coverage"],
    )
    parser.add_argument(
        "-f",
        "--force",
        help="disable all static analysis during the build (for experimenting)",
        action="store_true",
    )
    parser.add_argument(
        "-t",
        "--test",
        help="run tests after successfully building the project",
        action="store_true",
    )
    parser.add_argument(
        "-j",
        "--jobs",
        metavar="<num>",
        help="number of threads to parallelize the build",
        action="store",
        nargs="?",
        type=int,
        const=0,
    )
    parser.add_argument(
        "--compiler",
        metavar="<path>",
        help="override system's C++ compiler",
        action="store",
        default=None,
    )
    parser.add_argument(
        "--vcpkg-root",
        metavar="<path>",
        help="vcpkg package manager installation root path",
        action="store",
        default=None,
    )
    parser.add_argument(
        "--no-vcpkg",
        help="do not use vcpkg for building the project",
        action="store_true",
    )
    parser.add_argument(
        "--dry",
        help="perform a dry build: discard all previously built data.",
        action="store_true",
    )
    parser.add_argument(
        "--args",
        metavar="<args>",
        help="extra CMake configuration arguments",
        dest="extra_args",
        action="store",
        nargs=argparse.REMAINDER,
        default=None,
    )
    return parser.parse_args()


def prepare_build_dir(dry=False):
    """Prepare build directory."""
    if dry:
        print("Performing a dry build.")
        shutil.rmtree(OUTPUT_DIR, ignore_errors=True)
    pathlib.Path(OUTPUT_DIR).mkdir(parents=True, exist_ok=True)


def find_vcpkg(hint=None):
    """Find vcpkg installation"""
    # Use our hint first, next search environment.
    vcpkg_root = (
        hint
        or os.environ.get("VCPKG_ROOT")
        or os.environ.get("VCPKG_INSTALLATION_ROOT")
    )
    # Next look for common directories.
    if not vcpkg_root:
        maybe_vcpkg_root = os.path.join(HOME_DIR, "vcpkg")
        if os.path.exists(maybe_vcpkg_root) and os.path.isdir(maybe_vcpkg_root):
            vcpkg_root = maybe_vcpkg_root
    return vcpkg_root


def configure(
    config,
    force=False,
    compiler=None,
    no_vcpkg=False,
    vcpkg_root=None,
    extra_args=None,
):
    """Configure the project."""
    cmake_args = [CMAKE_EXE]
    # Setup the source and build directories.
    cmake_args += ["-S", SOURCE_DIR, "-B", OUTPUT_DIR]
    # Setup the build configuration.
    assert config
    print(f"Configuration: {config}.")
    cmake_args += ["-D" f"CMAKE_BUILD_TYPE={config}"]
    # Setup static analysis.
    # (Should be always passed, since `-D...` options are cached).
    if force:
        print("'Force' build: static analysis is disabled.")
    cmake_args += ["-D", f"SKIP_ANALYSIS={'YES' if force else 'NO'}"]
    # Setup C++ compiler.
    if compiler or os.environ.get("CXX"):
        print(f"Overriding system's C++ compiler: {compiler}.")
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
    if compiler:
        os.environ["CXX"] = compiler
    # Setup vcpkg.
    if not no_vcpkg:
        ## Find vcpkg.
        vcpkg_root = find_vcpkg(hint=vcpkg_root)
        if not vcpkg_root:
            print("Unable to find vcpkg.")
            sys.exit(-1)
        ## Locate toolchain file.
        toolchain_path = os.path.join(
            vcpkg_root, "scripts", "buildsystems", "vcpkg.cmake"
        )
        if not os.path.exists(toolchain_path) or not os.path.isfile(toolchain_path):
            print("Unable to find vcpkg toolchain file.")
            sys.exit(-1)
        cmake_args += ["-D", f"CMAKE_TOOLCHAIN_FILE={toolchain_path}"]
    # Append extra arguments.
    if extra_args:
        cmake_args += extra_args
    # Run CMake.
    if (exit_code := subprocess.call(cmake_args)) != 0:
        sys.exit(exit_code)


def build(config, jobs=0):
    """Build project."""
    cmake_args = ["cmake"]
    # Setup build directory.
    cmake_args += ["--build", OUTPUT_DIR]
    # Setup the build configuration.
    assert config
    cmake_args += ["--config", config]
    # Setup the number of threads.
    if jobs and jobs > 1:
        print(f"Building with {jobs} threads.")
        cmake_args += ["-j", jobs]
    # Run CMake.
    if (exit_code := subprocess.call(cmake_args)) != 0:
        sys.exit(exit_code)


def test(jobs=0):
    """Test project."""
    # Prepare CTest arguments.
    ctest_args = ["ctest", "--output-on-failure"]
    # Setup the number of threads.
    if jobs and jobs > 1:
        ctest_args += ["-j", jobs]
    # Run CTest.
    if (exit_code := subprocess.call(ctest_args, cwd=TEST_DIR)) != 0:
        sys.exit(exit_code)


def main():
    """Build Tit."""
    # Parse command line arguments.
    args = parse_args()

    # Prepare the build directory.
    prepare_build_dir(dry=args.dry)

    # Configure.
    configure(
        config=args.config,
        force=args.force,
        compiler=args.compiler,
        no_vcpkg=args.no_vcpkg,
        vcpkg_root=args.vcpkg_root,
    )

    # Build.
    build(config=args.config, jobs=args.jobs)

    # Test.
    if args.test:
        test(jobs=args.jobs)


if __name__ == "__main__":
    main()
