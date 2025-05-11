# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
# Commercial use, including SaaS, requires a separate license, see /LICENSE.md
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

add_tit_test(
  NAME "tit/core/unit"
  COMMAND "tit_core_tests"
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

add_tit_executable(
  NAME core_failed_check_test
  SOURCES "failed_check.cpp"
  DEPENDS tit::core
)

add_tit_test(
  NAME "tit/core/failed_check"
  COMMAND "tit_core_failed_check_test"
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

add_tit_test(
  NAME "tit/core/profiler"
  COMMAND "tit_core_profiler_tests"
  MATCH_STDOUT "profiler_stdout.txt"
  FILTERS "s/\\s*\\d+\\.\\d+/ <number>/g"
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

add_tit_executable(
  NAME core_stats_test
  SOURCES "stats.cpp"
  DEPENDS tit::core
)

add_tit_test(
  NAME "tit/core/stats"
  COMMAND "tit_core_stats_test"
  MATCH_STDOUT "stats_stdout.txt"
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
