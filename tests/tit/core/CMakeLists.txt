# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
# Part of the Tit Solver project, under the MIT License.
# See /LICENSE.md for license information. SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

add_test_from_target(
  tit::core_tests
  NAME "tit/core/unit")

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

tit_add_executable(
  NAME catch_ctrl_c_test
  SOURCES "catch_ctrl_c.cpp"
  DEPENDS core)

add_test_from_target(
  tit::catch_ctrl_c_test
  NAME "tit/core/catch_ctrl_c"
  EXIT_CODE 0
  MATCH_STDERR "catch_ctrl_c_stderr.txt")

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

tit_add_executable(
  NAME catch_fatal_signal_test
  SOURCES catch_fatal_signal.cpp
  DEPENDS core)

add_test_from_target(
  tit::catch_fatal_signal_test
  NAME "tit/core/catch_fatal_signal"
  EXIT_CODE 1
  MATCH_STDERR "catch_fatal_signal_stderr.txt"
  FILTERS "/0x*/d") # Remove everything related to the stack trace.

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

tit_add_executable(
  NAME profiler_test
  SOURCES profiler.cpp
  DEPENDS core)

add_test_from_target(
  tit::profiler_test
  NAME "tit/core/profiler"
  MATCH_STDOUT "profiler_stdout.txt"
  FILTERS "s/\\s*\\d+\\.\\d+/ <number>/g"
  PROPERTIES ENVIRONMENT "TIT_ENABLE_PROFILER=1")

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
