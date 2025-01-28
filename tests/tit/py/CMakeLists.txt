# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
# Commercial use, including SaaS, requires a separate license, see /LICENSE.md
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

add_tit_test(
  NAME "tit/py/unit"
  COMMAND "tit_py_tests"
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

add_tit_library(
  NAME test_module
  TYPE MODULE
  SUFFIX ".so"
  SOURCES "test_module.cpp"
  DEPENDS tit::py_module
)

add_tit_test(
  NAME "tit/py/test_module"
  COMMAND "titback" "test_script.py"
  INPUT_FILES "test_script.py"
  MATCH_STDOUT "test_module_stdout.txt"
  MATCH_STDERR "test_module_stderr.txt"
  FILTERS "s/0x[0-9a-f]+/<address>/g"
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
