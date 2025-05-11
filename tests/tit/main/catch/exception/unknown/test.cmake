# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
# Commercial use, including SaaS, requires a separate license, see /LICENSE.md
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

add_tit_test(
  EXE SOURCES "test.cpp" DEPENDS tit::main
  EXIT_CODE 134
  MATCH_STDERR "stderr.txt"
  FILTERS
    "/0x*/d" # Remove everything related to the stack trace.
    "/terminate called after throwing an instance of 'unsigned int'/d"
    "/terminating due to uncaught exception of type unsigned int/d"
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
