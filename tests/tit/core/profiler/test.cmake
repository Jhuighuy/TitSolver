# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
# Commercial use, including SaaS, requires a separate license, see /LICENSE.md
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

add_tit_executable(
  NAME core_profiler_test
  SOURCES "tit/core/profiler/test.cpp"
  DEPENDS tit::core
)

add_tit_test(
  COMMAND "tit_core_profiler_test"
  MATCH_STDOUT "stdout.txt"
  FILTERS "s/\\s*\\d+\\.\\d+/ <number>/g"
  ENVIRONMENT
    "TIT_ENABLE_PROFILER=1"
    "TIT_ENABLE_STATS=0"
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
