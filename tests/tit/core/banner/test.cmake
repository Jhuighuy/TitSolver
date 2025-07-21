# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
# Commercial use, including SaaS, requires a separate license, see /LICENSE.md
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

add_tit_test(
  EXE SOURCES "test.cpp" DEPENDS tit::core
  MATCH_STDOUT "stdout.txt"
  ENVIRONMENT "TIT_NO_BANNER=0"
  FILTERS "s/\\.\\.\\.\\. (.+)$/.... <string>/g" # Simplify system info.
  FILTERS "s/© (\\d+) - (\\d+)/© <YEAR> - <YEAR>/g" # Simplify copyright year.
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
