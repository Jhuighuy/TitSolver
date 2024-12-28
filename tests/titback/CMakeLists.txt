# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
# Commercial use, including SaaS, requires a separate license, see /LICENSE.md
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

add_tit_test(
  titback
  NAME "titback/exec"
  MATCH_STDOUT "exec_stdout.txt"
  COMMAND titback -c "import pytit $<SEMICOLON> print(pytit)"
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

add_tit_test(
  titback
  NAME "titback/exec_failure"
  EXIT_CODE 1
  MATCH_STDERR "exec_failure_stderr.txt"
  COMMAND titback -c "import does_not_exist"
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

add_tit_test(
  titback
  NAME "titback/exec_file"
  MATCH_STDOUT "exec_stdout.txt"
  INPUT_FILES "test.py"
  COMMAND titback test.py
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

add_tit_test(
  titback
  NAME "titback/exec_file_failure"
  EXIT_CODE 1
  MATCH_STDERR "exec_failure_file_stderr.txt"
  INPUT_FILES "invalid.py"
  COMMAND titback invalid.py
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

add_tit_test(
  titback
  NAME "titback/exec_missing_file"
  EXIT_CODE 1
  MATCH_STDERR "exec_missing_file_stderr.txt"
  COMMAND titback does_not_exist.py
  FILTERS "/0x*/d" # Remove everything related to the stack trace.
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
