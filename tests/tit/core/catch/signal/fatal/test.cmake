# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
# Commercial use, including SaaS, requires a separate license, see /LICENSE.md
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

add_tit_executable(
  NAME core_catch_signal_fatal_test
  SOURCES "tit/core/catch/signal/fatal/test.cpp"
  DEPENDS tit::core
)

add_tit_test(
  COMMAND "tit_core_catch_signal_fatal_test"
  EXIT_CODE 1
  MATCH_STDERR "stderr.txt"
  FILTERS "s/SIG.+$/<SIGNAL>./g" # Simplify signal name.
  FILTERS "/0x*/d" # Remove everything related to the stack trace.
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
