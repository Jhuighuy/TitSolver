# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
# Commercial use, including SaaS, requires a separate license, see /LICENSE.md
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

add_tit_test_executable(
  SOURCES "test.cpp"
  DEPENDS tit::core
  MATCH_STDOUT "stdout.txt"
  FILTERS "s/\\s*\\d+\\.\\d+/ <number>/g"
  ENVIRONMENT "TIT_ENABLE_PROFILER=1"
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
