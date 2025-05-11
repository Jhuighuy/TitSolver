# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
# Commercial use, including SaaS, requires a separate license, see /LICENSE.md
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

add_tit_executable(
  NAME main_catch_ctrl_c_test
  SOURCES "catch_ctrl_c.cpp"
  DEPENDS tit::core tit::main
)

add_tit_test(
  NAME "tit/main/catch_ctrl_c"
  COMMAND "tit_main_catch_ctrl_c_test"
  EXIT_CODE 254
  MATCH_STDERR "catch_ctrl_c_stderr.txt"
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

add_tit_executable(
  NAME main_catch_fatal_signal_test
  SOURCES "catch_fatal_signal.cpp"
  DEPENDS tit::core tit::main
)

add_tit_test(
  NAME "tit/main/catch_fatal_signal"
  COMMAND "tit_main_catch_fatal_signal_test"
  EXIT_CODE 245
  MATCH_STDERR "catch_fatal_signal_stderr.txt"
  FILTERS "/0x*/d" # Remove everything related to the stack trace.
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

add_tit_executable(
  NAME main_catch_std_exception_test
  SOURCES "catch_std_exception.cpp"
  DEPENDS tit::core tit::main
)

add_tit_test(
  NAME "tit/main/catch_std_exception"
  COMMAND "tit_main_catch_std_exception_test"
  EXIT_CODE 1
  MATCH_STDERR "catch_std_exception_stderr.txt"
  FILTERS "/basic_string/d" # Remove platform-specific error message.
  FILTERS "/0x*/d" # Remove everything related to the stack trace.
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

add_tit_executable(
  NAME main_catch_terminate_test
  SOURCES "catch_terminate.cpp"
  DEPENDS tit::core tit::main
)

add_tit_test(
  NAME "tit/main/catch_terminate"
  COMMAND "tit_main_catch_terminate_test"
  EXIT_CODE 1
  MATCH_STDERR "catch_terminate_stderr.txt"
  FILTERS "/0x*/d" # Remove everything related to the stack trace.
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

add_tit_executable(
  NAME main_catch_tit_exception_test
  SOURCES "catch_tit_exception.cpp"
  DEPENDS tit::core tit::main
)

add_tit_test(
  NAME "tit/main/catch_tit_exception"
  COMMAND "tit_main_catch_tit_exception_test"
  EXIT_CODE 1
  MATCH_STDERR "catch_tit_exception_stderr.txt"
  FILTERS "/0x*/d" # Remove everything related to the stack trace.
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

add_tit_executable(
  NAME main_catch_unknown_exception_test
  SOURCES "catch_unknown_exception.cpp"
  DEPENDS tit::core tit::main
)

add_tit_test(
  NAME "tit/main/catch_unknown_exception"
  COMMAND "tit_main_catch_unknown_exception_test"
  EXIT_CODE 134
  MATCH_STDERR "catch_unknown_exception_stderr.txt"
  FILTERS
    "/0x*/d" # Remove everything related to the stack trace.
    "/terminate called after throwing an instance of 'unsigned int'/d"
    "/terminating due to uncaught exception of type unsigned int/d"
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
