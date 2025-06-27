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
  cmake_parse_arguments(
    TEST
    ""
    "NAME;EXIT_CODE;STDIN;MATCH_STDOUT;MATCH_STDERR"
    "EXE;COMMAND;ENVIRONMENT;INPUT_FILES;MATCH_FILES;FILTERS;FLAGS"
    ${ARGN}
  )

  # Setup the test name.
  if(NOT TEST_NAME MATCHES "^[a-zA-Z0-9_]+(\\[[a-zA-Z0-9_]+\\])*$")
    message(FATAL_ERROR "Test name must be alphanumeric, with optional [tags].")
  endif()
  set(TEST_DIR "${CMAKE_CURRENT_LIST_DIR}")
  cmake_path(RELATIVE_PATH TEST_DIR)
  if(TEST_NAME)
    set(TEST_NAME "${TEST_DIR}/${TEST_NAME}")
  else()
    set(TEST_NAME "${TEST_DIR}")
  endif()

  # Setup the test command.
  if(TEST_EXE)
    if(TEST_COMMAND)
      message(FATAL_ERROR "Cannot specify both 'EXE' and 'COMMAND' arguments.")
    endif()
    cmake_parse_arguments(TEST "" "" "SOURCES;DEPENDS" ${TEST_EXE})

    # Setup the target name.
    string(REPLACE "/" "_" TEST_TARGET "${TEST_NAME}")
    set(TEST_TARGET "${TEST_TARGET}_test")

    # Setup the list of sources and dependencies. Since `test.cmake` files
    # are included in the root test directory, we need to provide the
    # absolute paths to the sources.
    if(NOT TEST_SOURCES)
      message(FATAL_ERROR "List of 'EXE' test sources must not be empty.")
    endif()
    list(TRANSFORM TEST_SOURCES PREPEND "${CMAKE_CURRENT_LIST_DIR}/")

    # Add the executable.
    add_tit_executable(
      NAME "${TEST_TARGET}"
      SOURCES ${TEST_SOURCES}
      DEPENDS ${TEST_DEPENDS}
    )

    # Test command will run the executable.
    set(TEST_COMMAND "${TEST_TARGET}")
  endif()
  if(NOT TEST_COMMAND)
    message(FATAL_ERROR "Command line must not be empty.")
  endif()

  # Setup the list of arguments for the test driver.
  set(TEST_DRIVER_ARGS "--name=${TEST_NAME}")
  if(TEST_EXIT_CODE)
    list(APPEND TEST_DRIVER_ARGS "--exit-code=${TEST_EXIT_CODE}")
  endif()
  if(TEST_STDIN)
    set(TEST_STDIN "${CMAKE_CURRENT_LIST_DIR}/${TEST_STDIN}")
    list(APPEND TEST_DRIVER_ARGS "--stdin=${TEST_STDIN}")
  endif()
  foreach(FILE ${TEST_INPUT_FILES})
    set(FILE "${CMAKE_CURRENT_LIST_DIR}/${FILE}")
    list(APPEND TEST_DRIVER_ARGS "--input-file=${FILE}")
  endforeach()
  if(TEST_MATCH_STDOUT)
    set(TEST_MATCH_STDOUT "${CMAKE_CURRENT_LIST_DIR}/${TEST_MATCH_STDOUT}")
    list(APPEND TEST_DRIVER_ARGS "--match-stdout=${TEST_MATCH_STDOUT}")
  endif()
  if(TEST_MATCH_STDERR)
    set(TEST_MATCH_STDERR "${CMAKE_CURRENT_LIST_DIR}/${TEST_MATCH_STDERR}")
    list(APPEND TEST_DRIVER_ARGS "--match-stderr=${TEST_MATCH_STDERR}")
  endif()
  foreach(FILE ${TEST_MATCH_FILES})
    set(FILE "${CMAKE_CURRENT_LIST_DIR}/${FILE}")
    list(APPEND TEST_DRIVER_ARGS "--match-file=${FILE}")
  endforeach()
  foreach(FILTER ${TEST_FILTERS})
    list(APPEND TEST_DRIVER_ARGS "--filter=${FILTER}")
  endforeach()
  list(APPEND TEST_DRIVER_ARGS "--" ${TEST_COMMAND})

  # Add the test.
  add_test(
    NAME "${TEST_NAME}"
    COMMAND "${TEST_DRIVER_EXE}" ${TEST_DRIVER_ARGS}
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
  )

  # Setup test environment.
  list(
    PREPEND
    TEST_ENVIRONMENT
    "TEST_DATA_DIR=${CMAKE_CURRENT_SOURCE_DIR}/_data"
    "TIT_ENABLE_PROFILER=0"
    "TIT_ENABLE_STATS=0"
  )
  set_tests_properties(
    "${TEST_NAME}"
    PROPERTIES ENVIRONMENT "${TEST_ENVIRONMENT}"
  )

  # Setup test flags.
  foreach(FLAG ${TEST_FLAGS})
    set_tests_properties("${TEST_NAME}" PROPERTIES "${FLAG}" TRUE)
  endforeach()
endfunction()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
