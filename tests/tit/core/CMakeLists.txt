# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
# Part of the Tit Solver project, under the MIT License.
# See /LICENSE.md for license information. SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

add_tit_executable(
  NAME
    core_tests
  SOURCES
    "math_utils.cpp"
    "mdvector.cpp"
    "meta.cpp"
    "system_utils.cpp"
    "time_utils.cpp"
    "vec.cpp"
  DEPENDS
    tit::core tit::doctest)

add_test_from_target(
  tit::core_tests
  NAME "tit::core::unit"
  EXIT_CODE 0)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

add_tit_executable(
  NAME core_catch_ctrl_c_test
  SOURCES "catch_ctrl_c.cpp"
  DEPENDS tit::core)

add_test_from_target(
  tit::core_catch_ctrl_c_test
  NAME "tit::core::catch_ctrl_c"
  EXIT_CODE 0
  MATCH_STDERR "catch_ctrl_c_stderr.txt")

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

add_tit_executable(
  NAME core_catch_fatal_signal_test
  SOURCES "catch_fatal_signal.cpp"
  DEPENDS tit::core)

add_test_from_target(
  tit::core_catch_fatal_signal_test
  NAME "tit::core::catch_fatal_signal"
  EXIT_CODE 1
  MATCH_STDERR "catch_fatal_signal_stderr.txt"
  FILTERS "/0x*/d") # Remove everything related to the stack trace.

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

add_tit_executable(
  NAME core_profiler_tests
  SOURCES "profiler.cpp"
  DEPENDS tit::core)

add_test_from_target(
  tit::core_profiler_tests
  NAME "tit::core::profiler"
  MATCH_STDOUT "profiler_stdout.txt"
  FILTERS "s/\\s*\\d+\\.\\d+/ <number>/g"
  PROPERTIES ENVIRONMENT "TIT_ENABLE_PROFILER=1")

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
