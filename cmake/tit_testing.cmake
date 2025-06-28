# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
# Commercial use, including SaaS, requires a separate license, see /LICENSE.md
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
# Register all the tests from `test.cmake` files in the current directory.
#
function(add_all_tit_tests)
  # Find all the test files and execute them.
  file(GLOB_RECURSE TEST_FILES "test.cmake")
  foreach(TEST_FILE ${TEST_FILES})
    include("${TEST_FILE}")
  endforeach()
endfunction()

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
    "COMMAND;ENVIRONMENT;INPUT_FILES;MATCH_FILES;FILTERS;FLAGS"
    ${ARGN}
  )
  if(NOT TEST_COMMAND)
    message(FATAL_ERROR "Command line must not be empty.")
  endif()
  set(TEST_NAME_REGEX "^[a-zA-Z0-9_]+(\\[[a-zA-Z0-9_]+\\])*$")
  if(TEST_NAME AND NOT TEST_NAME MATCHES "${TEST_NAME_REGEX}")
    message(FATAL_ERROR "Test name must be alphanumeric, with optional tags.")
  endif()

  # Build the test name.
  set(TEST_DIR "${CMAKE_CURRENT_LIST_DIR}")
  cmake_path(RELATIVE_PATH TEST_DIR)
  if(TEST_NAME)
    set(TEST_NAME "${TEST_DIR}/${TEST_NAME}")
  else()
    set(TEST_NAME "${TEST_DIR}")
  endif()

  # Build the list of arguments for the test driver.
  set(TEST_DRIVER_ARGS "--name=${TEST_NAME}")
  if(TEST_EXIT_CODE)
    list(APPEND TEST_DRIVER_ARGS "--exit-code=${TEST_EXIT_CODE}")
  endif()
  if(TEST_STDIN)
    set(TEST_STDIN "${CMAKE_CURRENT_LIST_DIR}/${TEST_STDIN}")
    cmake_path(ABSOLUTE_PATH TEST_STDIN NORMALIZE)
    list(APPEND TEST_DRIVER_ARGS "--stdin=${TEST_STDIN}")
  endif()
  foreach(FILE ${TEST_INPUT_FILES})
    set(FILE "${CMAKE_CURRENT_LIST_DIR}/${FILE}")
    cmake_path(ABSOLUTE_PATH FILE NORMALIZE)
    list(APPEND TEST_DRIVER_ARGS "--input-file=${FILE}")
  endforeach()
  if(TEST_MATCH_STDOUT)
    set(TEST_MATCH_STDOUT "${CMAKE_CURRENT_LIST_DIR}/${TEST_MATCH_STDOUT}")
    cmake_path(ABSOLUTE_PATH TEST_MATCH_STDOUT NORMALIZE)
    list(APPEND TEST_DRIVER_ARGS "--match-stdout=${TEST_MATCH_STDOUT}")
  endif()
  if(TEST_MATCH_STDERR)
    set(TEST_MATCH_STDERR "${CMAKE_CURRENT_LIST_DIR}/${TEST_MATCH_STDERR}")
    cmake_path(ABSOLUTE_PATH TEST_MATCH_STDERR NORMALIZE)
    list(APPEND TEST_DRIVER_ARGS "--match-stderr=${TEST_MATCH_STDERR}")
  endif()
  foreach(FILE ${TEST_MATCH_FILES})
    set(FILE "${CMAKE_CURRENT_LIST_DIR}/${FILE}")
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

  # Setup test environment.
  # TODO: We need proper environment sandboxing.
  list(
    PREPEND
    TEST_ENVIRONMENT
    "TEST_DATA_DIR=${CMAKE_CURRENT_SOURCE_DIR}/_data"
    "TIT_ENABLE_PROFILER=0"
    "TIT_ENABLE_STATS=0"
  )
  set_tests_properties(
    "${TEST_NAME}"
    PROPERTIES
    ENVIRONMENT
    "${TEST_ENVIRONMENT}"
  )

  # Setup test flags.
  foreach(FLAG ${TEST_FLAGS})
    set_tests_properties("${TEST_NAME}" PROPERTIES "${FLAG}" TRUE)
  endforeach()
endfunction()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#
# Add a test executable and register it as a test.
#
function(add_tit_test_executable)
  # Parse and check arguments.
  cmake_parse_arguments(
    TEST
    ""
    "NAME;EXIT_CODE;STDIN;MATCH_STDOUT;MATCH_STDERR"
    "SOURCES;DEPENDS;ENVIRONMENT;INPUT_FILES;MATCH_FILES;FILTERS;FLAGS"
    ${ARGN}
  )
  if(NOT TEST_SOURCES)
    message(FATAL_ERROR "List of sources must not be empty.")
  endif()

  # Build the test executable name.
  # TODO: Do not duplicate the logic from `add_tit_test`!
  set(TEST_DIR "${CMAKE_CURRENT_LIST_DIR}")
  cmake_path(RELATIVE_PATH TEST_DIR)
  if(TEST_NAME)
    set(TEST_NAME "${TEST_DIR}/${TEST_NAME}")
  else()
    set(TEST_NAME "${TEST_DIR}")
  endif()
  string(REPLACE "/" "_" TEST_NAME "${TEST_NAME}")
  set(TEST_NAME "${TEST_NAME}_test")

  # Build the of sources.
  list(TRANSFORM TEST_SOURCES PREPEND "${CMAKE_CURRENT_LIST_DIR}/")

  # Create the test executable.
  add_tit_executable(
    NAME "${TEST_NAME}"
    SOURCES ${TEST_SOURCES}
    DEPENDS ${TEST_DEPENDS}
  )

  # Add the test.
  add_tit_test(
    COMMAND "${TEST_NAME}"
    EXIT_CODE ${TEST_EXIT_CODE}
    STDIN ${TEST_STDIN}
    MATCH_STDOUT ${TEST_MATCH_STDOUT}
    MATCH_STDERR ${TEST_MATCH_STDERR}
    ENVIRONMENT ${TEST_ENVIRONMENT}
    INPUT_FILES ${TEST_INPUT_FILES}
    MATCH_FILES ${TEST_MATCH_FILES}
    FILTERS ${TEST_FILTERS}
    FLAGS ${TEST_FLAGS}
  )
endfunction()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
