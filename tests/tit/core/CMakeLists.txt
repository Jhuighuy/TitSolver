# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
# Part of the Tit Solver project, under the MIT License.
# See /LICENSE.md for license information. SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

# Specify the list of sources.
set(
  CXX_SOURCES
  "math.cpp"
  "mdvector.cpp"
  "meta.cpp"
  "posix_utils.cpp"
  "string_utils.cpp"
  "time_utils.cpp"
  "vec.cpp")

# Create the unit test executable.
add_doctest_executable(tit_core_tests ${CXX_SOURCES})
add_executable(tit::core_tests ALIAS tit_core_tests)

# Link with the dependent libraries.
target_link_libraries(tit_core_tests PRIVATE tit::core)

# Enable static analysis.
enable_clang_tidy(tit::core_tests)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

# Create the test executable.
add_executable(tit_core_catch_ctrl_c_test "catch_ctrl_c.cpp")
add_executable(tit::core_catch_ctrl_c_test ALIAS tit_core_catch_ctrl_c_test)

# Link with the dependent libraries.
target_link_libraries(tit_core_catch_ctrl_c_test PRIVATE tit::core)

# Enable static analysis.
enable_clang_tidy(tit::core_catch_ctrl_c_test)

# Register the test.
add_test_from_target(
  tit::core_catch_ctrl_c_test
  NAME "tit/core/catch_ctrl_c"
  EXIT_CODE 0
  MATCH_STDERR "catch_ctrl_c_stderr.txt")

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

# Create the test executable.
add_executable(tit_core_catch_fatal_signal_test "catch_fatal_signal.cpp")
add_executable(
  tit::core_catch_fatal_signal_test ALIAS
  tit_core_catch_fatal_signal_test)

# Link with the dependent libraries.
target_link_libraries(tit_core_catch_fatal_signal_test PRIVATE tit::core)

# Enable static analysis.
enable_clang_tidy(tit::core_catch_fatal_signal_test)

# Register the test.
add_test_from_target(
  tit::core_catch_fatal_signal_test
  NAME "tit/core/catch_fatal_signal"
  EXIT_CODE 1
  MATCH_STDERR "catch_fatal_signal_stderr.txt"
  # Remove everything related to the stack trace.
  FILTERS "/0x*/d")

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

# Create the test executable.
add_executable(tit_core_profiler_tests "profiler.cpp")
add_executable(tit::core_profiler_tests ALIAS tit_core_profiler_tests)

# Link with the dependent libraries.
target_link_libraries(tit_core_profiler_tests PRIVATE tit::core)

# Enable static analysis.
enable_clang_tidy(tit::core_profiler_tests)

# Register the test.
add_test_from_target(
  tit::core_profiler_tests
  NAME "tit/core/profiler"
  MATCH_STDOUT "profiler_stdout.txt"
  FILTERS "s/\\s*\\d+\\.\\d+/ <number>/g"
  PROPERTIES ENVIRONMENT "TIT_ENABLE_PROFILER=1")

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
