# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
# Commercial use, including SaaS, requires a separate license, see /LICENSE.md
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Check that the exit code is matched correctly.
add_tit_test(
  NAME "test_driver/match_exit_code"
  COMMAND "${BASH_EXE}" -c "exit -1"
  EXIT_CODE "-1"
)

add_tit_test(
  NAME "test_driver/match_exit_code_failure"
  COMMAND "${BASH_EXE}" -c "exit -1"
)
set_tests_properties(
  "test_driver/match_exit_code_failure"
  PROPERTIES WILL_FAIL TRUE
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Check that the standard input is provided.
add_tit_test(
  NAME "test_driver/check_stdin"
  STDIN "input_1.txt"
  COMMAND "${BASH_EXE}" -c "test -s /dev/stdin"
)

# Check that the input files are provided.
#
# Note: we cannot use `; as the command separator since it is treated
# as the argument separator by CMake. Instead, we use `&&` to separate
# commands.
add_tit_test(
  NAME "test_driver/check_input_files"
  INPUT_FILES "input_1.txt" "input_2.txt"
  COMMAND "${BASH_EXE}" -c "stat ./input_1.txt && stat ./input_2.txt"
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Check that the standard output streams are captured correctly.
add_tit_test(
  NAME "test_driver/check_stdout"
  MATCH_STDOUT "output_1.txt"
  COMMAND "${BASH_EXE}" -c "echo 'Some output.'"
)
add_tit_test(
  NAME "test_driver/check_stderr"
  MATCH_STDERR "output_2.txt"
  COMMAND "${BASH_EXE}" -c "echo 'Some different output.' 1>&2"
)

# Check that the output files are captured correctly.
add_tit_test(
  NAME "test_driver/check_output_files"
  MATCH_FILES "output_1.txt" "output_2.txt"
  COMMAND "${BASH_EXE}" -c
    "echo 'Some output.' > ./output_1.txt &&
     echo 'Some different output.' > ./output_2.txt"
)

# Check that the output files with '.checksum' suffix are matched correctly.
add_tit_test(
  NAME "test_driver/check_output_file_checksum"
  MATCH_FILES "output_1.txt.checksum"
  COMMAND "${BASH_EXE}" -c "echo 'Some output.' > ./output_1.txt"
)

add_tit_test(
  NAME "test_driver/check_output_file_checksum_failure"
  MATCH_FILES "output_1.txt.checksum"
  COMMAND "${BASH_EXE}" -c "echo 'Some erroneous output.' > ./output_1.txt"
)
set_tests_properties(
  "test_driver/check_output_file_checksum_failure"
  PROPERTIES WILL_FAIL TRUE
)

# Check that the missing output files are detected.
add_tit_test(
  NAME "test_driver/missing_output_file_1"
  MATCH_FILES "output_1.txt"
  COMMAND "${BASH_EXE}" -c "exit 0"
)
set_tests_properties(
  "test_driver/missing_output_file_1"
  PROPERTIES WILL_FAIL TRUE
)

add_tit_test(
  NAME "test_driver/missing_output_file_2"
  MATCH_FILES "output_1.txt.checksum"
  COMMAND "${BASH_EXE}" -c "exit 0"
)
set_tests_properties(
  "test_driver/missing_output_file_2"
  PROPERTIES WILL_FAIL TRUE
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Check that the trailing whitespaces are ignored when matching files.
add_tit_test(
  NAME "test_driver/ignore_trailing_whitespaces"
  MATCH_STDOUT "output_1.txt"
  COMMAND "${BASH_EXE}" -c "echo 'Some output.        '"
)

# Check that the empty lines are ignored when matching files.
add_tit_test(
  NAME "test_driver/ignore_empty_lines"
  MATCH_STDOUT "output_with_empty_lines.txt"
  COMMAND "${BASH_EXE}" -c
    "printf 'First line.\n\n\n' &&
     printf 'Second line.\n\n' &&
     printf 'Third line.\n\n\n\n' &&
     printf 'Fourth line.\n'"
)

# Check that all the escape sequences are correctly filtered.
add_tit_test(
  NAME "test_driver/ignore_escapes"
  MATCH_STDOUT "output_1.txt"
  COMMAND "${BASH_EXE}" -c "printf '\\033[1mSome \\033[94moutput\\033[0m.\n'"
)

# Check that the full paths in the output files are correctly filtered.
add_tit_test(
  NAME "test_driver/match_files_with_paths"
  MATCH_FILES "output_with_paths.txt"
  COMMAND "${BASH_EXE}" -c
    "echo Ensure /file/name/should.stay >> ./output_with_paths.txt &&
     echo Ensure relative/paths/are.untouched >> ./output_with_paths.txt"
)

# Check that custom filters work.
add_tit_test(
  NAME "test_driver/custom_filter"
  MATCH_STDOUT "output_1.txt"
  FILTERS "s/#.*$//g"
  COMMAND "${BASH_EXE}" -c
    "echo '# This comment should be filtered away.' &&
     echo 'Some output. # Another comment.'"
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Exit code is correct, but a single output files is not.
add_tit_test(
  NAME "test_driver/integration_test_1"
  MATCH_FILES "output_1.txt" "output_2.txt"
  COMMAND "${BASH_EXE}" -c
    "echo 'Some output.' > ./output_1.txt &&
     echo 'Some different erroneous output.' > ./output_2.txt"
)
set_tests_properties(
  "test_driver/integration_test_1"
  PROPERTIES WILL_FAIL TRUE
)

# All output files are correct, but a file is missing.
add_tit_test(
  NAME "test_driver/integration_test_2"
  MATCH_FILES "output_1.txt" "output_2.txt" "output_3.txt"
  COMMAND "${BASH_EXE}" -c
    "echo 'Some output.' > ./output_1.txt &&
     echo 'Some different erroneous output.' > ./output_2.txt"
)
set_tests_properties(
  "test_driver/integration_test_2"
  PROPERTIES WILL_FAIL TRUE
)

# All output files are correct, but exit code is not.
add_tit_test(
  NAME "test_driver/integration_test_3"
  MATCH_FILES "output_1.txt" "output_2.txt"
  COMMAND "${BASH_EXE}" -c
    "echo 'Some output.' > ./output_1.txt &&
     echo 'Some different output.' > ./output_2.txt &&
     exit -1"
)
set_tests_properties(
  "test_driver/integration_test_3"
  PROPERTIES WILL_FAIL TRUE
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
