# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of the Tit Solver project, under the MIT License.
# See /LICENSE.md for license information. SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

include_guard()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Enable CTest.
enable_testing()

# Find test driver executable.
find_program(
  TEST_DRIVER_EXE
  NAMES "test-driver.sh"
  PATHS "${CMAKE_SOURCE_DIR}/build"
  REQUIRED
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#
# Register the test command.
#
function(add_tit_test)
  # Parse and check arguments.
  cmake_parse_arguments(
    TEST
    ""
    "NAME;EXIT_CODE;STDIN;MATCH_STDOUT;MATCH_STDERR"
    "COMMAND;ENVIRONMENT;INPUT_FILES;MATCH_FILES;FILTERS"
    ${ARGN}
  )
  if(NOT TEST_NAME)
    message(FATAL_ERROR "Test name must be specified.")
  endif()
  if(NOT TEST_COMMAND)
    message(FATAL_ERROR "Command line must not be empty.")
  endif()

  # Build the list of arguments for the test driver.
  set(TEST_DRIVER_ARGS "--name=${TEST_NAME}")
  if(TEST_EXIT_CODE)
    list(APPEND TEST_DRIVER_ARGS "--exit-code=${TEST_EXIT_CODE}")
  endif()
  if(TEST_STDIN)
    cmake_path(ABSOLUTE_PATH TEST_STDIN NORMALIZE)
    list(APPEND TEST_DRIVER_ARGS "--stdin=${TEST_STDIN}")
  endif()
  foreach(FILE ${TEST_INPUT_FILES})
    cmake_path(ABSOLUTE_PATH FILE NORMALIZE)
    list(APPEND TEST_DRIVER_ARGS "--input-file=${FILE}")
  endforeach()
  if(TEST_MATCH_STDOUT)
    cmake_path(ABSOLUTE_PATH TEST_MATCH_STDOUT NORMALIZE)
    list(APPEND TEST_DRIVER_ARGS "--match-stdout=${TEST_MATCH_STDOUT}")
  endif()
  if(TEST_MATCH_STDERR)
    cmake_path(ABSOLUTE_PATH TEST_MATCH_STDERR NORMALIZE)
    list(APPEND TEST_DRIVER_ARGS "--match-stderr=${TEST_MATCH_STDERR}")
  endif()
  foreach(FILE ${TEST_MATCH_FILES})
    cmake_path(ABSOLUTE_PATH FILE NORMALIZE)
    list(APPEND TEST_DRIVER_ARGS "--match-file=${FILE}")
  endforeach()
  foreach(FILTER ${TEST_FILTERS})
    list(APPEND TEST_DRIVER_ARGS "--filter=${FILTER}")
  endforeach()
  list(APPEND TEST_DRIVER_ARGS "--" ${TEST_COMMAND})

  # Register the test.
  add_test(
    NAME "${TEST_NAME}"
    COMMAND "${TEST_DRIVER_EXE}" ${TEST_DRIVER_ARGS}
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
  )

  # Set test environment.
  if(TEST_ENVIRONMENT)
    set_tests_properties(
      "${TEST_NAME}"
      PROPERTIES ENVIRONMENT "${TEST_ENVIRONMENT}"
    )
  endif()
endfunction()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#
# Register the test target.
#
function(add_tit_test_from_target TARGET)
  # Parse and check arguments.
  if(NOT TARGET)
    message(FATAL_ERROR "Target must be specified.")
  endif()
  cmake_parse_arguments(
    TEST
    ""
    "NAME;EXIT_CODE;STDIN;MATCH_STDOUT;MATCH_STDERR"
    "ARGS;ENVIRONMENT;INPUT_FILES;MATCH_FILES;FILTERS"
    ${ARGN}
  )

  # Add the test for this executable.
  add_tit_test(
    NAME ${TEST_NAME}
    COMMAND "$<TARGET_FILE:${TARGET}>" ${TEST_ARGS}
    EXIT_CODE ${TEST_EXIT_CODE}
    STDIN ${TEST_STDIN}
    ENVIRONMENT ${TEST_ENVIRONMENT}
    INPUT_FILES ${TEST_INPUT_FILES}
    MATCH_STDOUT ${TEST_MATCH_STDOUT}
    MATCH_STDERR ${TEST_MATCH_STDERR}
    MATCH_FILES ${TEST_MATCH_FILES}
    FILTERS ${TEST_FILTERS}
  )
endfunction()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
