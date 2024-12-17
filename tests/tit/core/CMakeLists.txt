# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of the Tit Solver project, under the MIT License.
# See /LICENSE.md for license information. SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

add_tit_test_from_target(
  tit::core_tests
  NAME "tit/core/unit"
  ENVIRONMENT
    "TEST_ZERO=0"
    "TEST_POS_INT=123"
    "TEST_NEG_INT=-456"
    "TEST_TRUE=TRUE"
    "TEST_FALSE=false"
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

add_tit_executable(
  NAME core_catch_ctrl_c_test
  SOURCES "catch_ctrl_c.cpp"
  DEPENDS tit::core
)

add_tit_test_from_target(
  tit::core_catch_ctrl_c_test
  NAME "tit/core/catch_ctrl_c"
  MATCH_STDERR "catch_ctrl_c_stderr.txt"
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

add_tit_executable(
  NAME core_catch_fatal_signal_test
  SOURCES "catch_fatal_signal.cpp"
  DEPENDS tit::core
)

add_tit_test_from_target(
  tit::core_catch_fatal_signal_test
  NAME "tit/core/catch_fatal_signal"
  EXIT_CODE 1
  MATCH_STDERR "catch_fatal_signal_stderr.txt"
  FILTERS "s/SIG.+$/<SIGNAL>./g" # Simplify signal name.
  FILTERS "/0x*/d" # Remove everything related to the stack trace.
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

add_tit_executable(
  NAME core_catch_std_exception_test
  SOURCES "catch_std_exception.cpp"
  DEPENDS tit::core
)

add_tit_test_from_target(
  tit::core_catch_std_exception_test
  NAME "tit/core/catch_std_exception"
  EXIT_CODE 1
  MATCH_STDERR "catch_std_exception_stderr.txt"
  FILTERS "/basic_string/d" # Remove platform-specific error message.
  FILTERS "/0x*/d" # Remove everything related to the stack trace.
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

add_tit_executable(
  NAME core_catch_terminate_test
  SOURCES "catch_terminate.cpp"
  DEPENDS tit::core
)

add_tit_test_from_target(
  tit::core_catch_terminate_test
  NAME "tit/core/catch_terminate"
  EXIT_CODE 1
  MATCH_STDERR "catch_terminate_stderr.txt"
  FILTERS "/0x*/d" # Remove everything related to the stack trace.
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

add_tit_executable(
  NAME core_catch_tit_exception_test
  SOURCES "catch_tit_exception.cpp"
  DEPENDS tit::core
)

add_tit_test_from_target(
  tit::core_catch_tit_exception_test
  NAME "tit/core/catch_tit_exception"
  EXIT_CODE 1
  MATCH_STDERR "catch_tit_exception_stderr.txt"
  FILTERS "/0x*/d" # Remove everything related to the stack trace.
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

add_tit_executable(
  NAME core_catch_unknown_exception_test
  SOURCES "catch_unknown_exception.cpp"
  DEPENDS tit::core
)

add_tit_test_from_target(
  tit::core_catch_unknown_exception_test
  NAME "tit/core/catch_unknown_exception"
  EXIT_CODE 1
  MATCH_STDERR "catch_unknown_exception_stderr.txt"
  FILTERS "/0x*/d" # Remove everything related to the stack trace.
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

add_tit_executable(
  NAME core_failed_check_test
  SOURCES "failed_check.cpp"
  DEPENDS tit::core
)

add_tit_test_from_target(
  tit::core_failed_check_test
  NAME "tit/core/failed_check"
  EXIT_CODE 1
  MATCH_STDERR "failed_check_stderr.txt"
  FILTERS "/0x*/d" # Remove everything related to the stack trace.
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

add_tit_executable(
  NAME core_profiler_tests
  SOURCES "profiler.cpp"
  DEPENDS tit::core
)

add_tit_test_from_target(
  tit::core_profiler_tests
  NAME "tit/core/profiler"
  MATCH_STDOUT "profiler_stdout.txt"
  FILTERS "s/\\s*\\d+\\.\\d+/ <number>/g"
  ENVIRONMENT
    "TIT_ENABLE_PROFILER=1"
    "TIT_ENABLE_STATS=0"
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

add_tit_executable(
  NAME core_stats_test
  SOURCES "stats.cpp"
  DEPENDS tit::core
)

add_tit_test_from_target(
  tit::core_stats_test
  NAME "tit/core/stats"
  MATCH_STDOUT "stats_stdout.txt"
  ENVIRONMENT
    "TIT_ENABLE_PROFILER=0"
    "TIT_ENABLE_STATS=1"
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
