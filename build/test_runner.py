#!/usr/bin/env python3
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
# Part of the Tit Solver project, under the MIT License.
# See /LICENSE.md for license information. SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

import argparse
import os
import pathlib
import platform
import shutil
import subprocess
import sys

APPLE = platform.system() == "Darwin"
SOURCE_DIR = os.path.abspath(".")
ROOT_TEST_DIR = os.path.join(SOURCE_DIR, "output", "test_output")

DIFF_EXE = "diff"
SED_EXE = "gsed" if APPLE else "sed"
SED_FILTERS = [
    # Shrink absolute paths to filenames.
    r"s/\/(([^\/]+|\.{1,2})\/)+([^\/]+)/\3/g",
    # Shrink source locations.
    r"s/(\w+\.\w+):\d+:\d+/\1:<line>:<column>/g",
    # Remove escape sequences.
    r"/\x1B\[(\d+;)*\d+[mGK]/d",
    # Remove profiling reports.
    r"/^libgcov profiling error:.+$/d",
]


def parse_args():
    """Parse command line arguments."""
    parser = argparse.ArgumentParser(description=main.__doc__)
    parser.add_argument(
        "--test-name",
        help="test name",
        dest="name",
        required=True,
    )
    parser.add_argument(
        "--test-exit-code",
        metavar="<exit-code>",
        help="expected test exit code",
        type=int,
        default=0,
        action="store",
        dest="exit_code",
    )
    parser.add_argument(
        "--test-stdin",
        metavar="<path>",
        help="provide input for the test",
        action="store",
        dest="stdin",
    )
    parser.add_argument(
        "--test-input-file",
        metavar="<path>",
        help="provide input file for the test",
        default=[],
        action="append",
        dest="input_files",
    )
    parser.add_argument(
        "--test-match-stdout",
        metavar="<path>",
        help="match test 'stdout' with the specified file",
        action="store",
        dest="stdout",
    )
    parser.add_argument(
        "--test-match-stderr",
        metavar="<path>",
        help="match test 'stderr' with the specified file",
        action="store",
        dest="stderr",
    )
    parser.add_argument(
        "--test-match-file",
        metavar="<path>",
        help="match test output file with the specified file",
        default=[],
        action="append",
        dest="match_files",
    )
    parser.add_argument(
        "--test-command",
        help="test command line arguments",
        required=True,
        action="store",
        nargs=argparse.REMAINDER,
        dest="command",
    )
    return parser.parse_args()


def add_suffix(path, suffix):
    """Add suffix to the filename before extension"""
    assert path and suffix
    path_we, ext = os.path.splitext(path)
    return f"{path_we}_{suffix}{ext}"


def prepare_test_dir(test_dir):
    """Prepare the test output directory."""
    assert test_dir
    # Remove the existing directory.
    shutil.rmtree(test_dir, ignore_errors=True)
    # Create a brand-new directory.
    pathlib.Path(test_dir).mkdir(parents=True, exist_ok=True)


def prepare_input_file(test_dir, external_path, filename=None):
    """Create a symlink to the file inside of the test directory."""
    assert test_dir
    if not external_path:
        return None
    if not filename:
        _, filename = os.path.split(external_path)
    path = os.path.join(test_dir, filename)
    os.symlink(external_path, path)
    return path


def prepare_output_file(test_dir, external_path, filename=None):
    """Create a prefixed symlink to the file inside of the test directory,
    also prepare a name for the actual output."""
    assert test_dir
    if not external_path:
        return None, None
    if not filename:
        _, filename = os.path.split(external_path)
    expected_path = os.path.join(test_dir, add_suffix(filename, "expected"))
    actual_path = os.path.join(test_dir, filename)
    os.symlink(external_path, expected_path)
    return expected_path, actual_path


def run_test(
    command, test_dir, stdin_path=None, stdout_path=None, stderr_path=None
):
    """Run the test, providing stdin to it and intercepting it's
    stdout and stderr."""
    assert command and test_dir
    streams = {
        "stdin": (stdin_path, "r"),
        "stdout": (stdout_path, "w"),
        "stderr": (stderr_path, "w"),
    }
    try:
        streams = {
            name: open(path, mode, encoding="utf-8")
            for name, (path, mode) in streams.items()
            if path
        }
        return subprocess.call(args=command, cwd=test_dir, **streams)
    finally:
        # Tidy-up the streams.
        for fd in streams.values():
            fd.close()


def match_exit_code(expected_exit_code, actual_exit_code):
    """Check that exit codes match."""
    return expected_exit_code & 0xFF == actual_exit_code & 0xFF


def filter_file(path):
    """Filter file contents."""
    assert path
    # Prepare the filtered file.
    filtered_path = add_suffix(path, "filtered")
    shutil.copyfile(path, filtered_path)
    # Apply the filters.
    for sed_filter in SED_FILTERS:
        sed_filter = sed_filter.replace(r"\d", "[0-9]").replace(
            r"\w", "[A-Za-z_]"
        )
        subprocess.check_call([SED_EXE, "-rE", sed_filter, "-i", filtered_path])
    return filtered_path


def match_file(expected_path, actual_path):
    """Check that files match."""
    if not expected_path:
        assert not actual_path
        return True, None
    filtered_expected_path = filter_file(expected_path)
    filtered_actual_path = filter_file(actual_path)
    diff_path = actual_path + ".diff"
    with open(diff_path, "w", encoding="utf-8") as diff_fd:
        exit_code = subprocess.call(
            [
                DIFF_EXE,
                "--ignore-blank-lines",
                "--ignore-trailing-space",
                filtered_expected_path,
                filtered_actual_path,
            ],
            stdout=diff_fd,
        )
    return exit_code == 0, diff_path


def main():
    """Test driver: run the test and match output."""
    # Parse arguments.
    args = parse_args()

    # Prepare the test directory.
    test_name = args.name
    test_dir = os.path.join(ROOT_TEST_DIR, test_name.replace("::", "/"))
    prepare_test_dir(test_dir)

    # Prepare the test input and output.
    expected_exit_code = args.exit_code
    ## Prepare input files.
    stdin = prepare_input_file(test_dir, args.stdin, "stdin.txt")
    for path in args.input_files:
        prepare_input_file(test_dir, path)
    ## Prepare output files.
    expected_stdout, actual_stdout = prepare_output_file(
        test_dir, args.stdout, "stdout.txt"
    )
    expected_stderr, actual_stderr = prepare_output_file(
        test_dir, args.stderr, "stderr.txt"
    )
    output_files = [
        prepare_output_file(test_dir, path) for path in args.match_files
    ]
    output_files += [
        (expected_stdout, actual_stdout),
        (expected_stderr, actual_stderr),
    ]

    # Run the test.
    command = args.command
    actual_exit_code = run_test(
        command, test_dir, stdin, actual_stdout, actual_stderr
    )

    # Match the results.
    test_passed = True

    # Match exit code.
    matched = match_exit_code(expected_exit_code, actual_exit_code)
    if not matched:
        print(
            f"Exit codes does not match: "
            f"expected {expected_exit_code}, got {actual_exit_code}."
        )
        test_passed = False

    # Match output files.
    for expected_path, actual_path in output_files:
        matched, diff_path = match_file(expected_path, actual_path)
        if not matched:
            _, actual_filename = os.path.split(actual_path)
            print(f"File '{actual_filename}' does not match. See: {diff_path}")
            print(f"Note: actual output: {actual_path}")
            test_passed = False

    # Exit with result.
    if test_passed:
        print("Test passed.")
        sys.exit(0)
    else:
        print("Test failed.")
        sys.exit(-1)


if __name__ == "__main__":
    main()
