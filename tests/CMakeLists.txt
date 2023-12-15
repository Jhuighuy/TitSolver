# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
# Part of the Tit Solver project, under the MIT License.
# See /LICENSE.md for license information. SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

# Enable CTest.
enable_testing()

# Setup test root directory.
set(TEST_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

# Find doctest.
find_package(doctest CONFIG REQUIRED)
include(doctest)

# Create a base library for the doctest-based test executable.
add_library(tit_doctest OBJECT "doctest_main.cpp")
add_library(tit::doctest ALIAS tit_doctest)

# Link with doctest.
target_link_libraries(tit_doctest PUBLIC doctest::doctest)

## Add a test executable target that uses doctest as the underlying framework.
## All the actual tests inside of the executable are detected automatically.
## Interface of this function is pretty identical to normal `add_executable`.
function(add_unit_test_executable TARGET)
  # Create the executable.
  add_executable(${TARGET} ${ARGN})
  # Link with doctest.
  target_link_libraries(${TARGET} PRIVATE tit::doctest)
  # Register the tests.
  doctest_discover_tests(
    ${TARGET}
    EXTRA_ARGS --force-colors=true
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
endfunction()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

# Find test runner executable.
find_program(
  TEST_RUNNER_EXE
  NAMES "test_runner.py"
  PATHS "${CMAKE_SOURCE_DIR}/build"
  REQUIRED)

## Add command that would be executed during testing.
function(add_test_command)
  # Parse and check arguments.
  set(OPTIONS)
  set(ONE_VALUE_ARGS NAME EXIT_CODE STDIN MATCH_STDOUT MATCH_STDERR)
  set(MULTI_VALUE_ARGS COMMAND INPUT_FILES MATCH_FILES PROPERTIES)
  cmake_parse_arguments(
    TEST
    "${OPTIONS}"
    "${ONE_VALUE_ARGS}"
    "${MULTI_VALUE_ARGS}"
    ${ARGN})
  if (NOT TEST_NAME)
    message(FATAL_ERROR "Test name must be specified.")
  endif()
  if (NOT TEST_COMMAND)
    message(FATAL_ERROR "Command line must not be empty.")
  endif()
  # Prepare the list of arguments for the test driver.
  set(TEST_DRIVER_ARGS "--test-name=${TEST_NAME}")
  if(TEST_EXIT_CODE)
    list(APPEND TEST_DRIVER_ARGS "--test-exit-code=${TEST_EXIT_CODE}")
  endif()
  if(TEST_STDIN)
    cmake_path(ABSOLUTE_PATH TEST_STDIN NORMALIZE)
    list(APPEND TEST_DRIVER_ARGS "--test-stdin=${TEST_STDIN}")
  endif()
  foreach(FILE ${TEST_INPUT_FILES})
    cmake_path(ABSOLUTE_PATH FILE NORMALIZE)
    list(APPEND TEST_DRIVER_ARGS "--test-input-file=${FILE}")
  endforeach()
  if(TEST_MATCH_STDOUT)
    cmake_path(ABSOLUTE_PATH TEST_MATCH_STDOUT NORMALIZE)
    list(APPEND TEST_DRIVER_ARGS "--test-match-stdout=${TEST_MATCH_STDOUT}")
  endif()
  if(TEST_MATCH_STDERR)
    cmake_path(ABSOLUTE_PATH TEST_MATCH_STDERR NORMALIZE)
    list(APPEND TEST_DRIVER_ARGS "--test-match-stderr=${TEST_MATCH_STDERR}")
  endif()
  foreach(FILE ${TEST_MATCH_FILES})
    cmake_path(ABSOLUTE_PATH FILE NORMALIZE)
    list(APPEND TEST_DRIVER_ARGS "--test-match-file=${FILE}")
  endforeach()
  list(APPEND TEST_DRIVER_ARGS "--test-command" ${TEST_COMMAND})
  # Register the test.
  add_test(
    NAME "${TEST_NAME}"
    COMMAND "${TEST_RUNNER_EXE}" ${TEST_DRIVER_ARGS}
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
  # Set test properties.
  if(TEST_PROPERTIES)
    set_tests_properties("${TEST_NAME}" PROPERTIES ${TEST_PROPERTIES})
  endif()
endfunction()

## Add command that would executes the target during testing.
function(add_test_command_from_target TARGET)
  # Parse and check arguments.
  if(NOT TARGET)
    message(FATAL_ERROR "Target must be specified.")
  endif()
  set(OPTIONS)
  set(ONE_VALUE_ARGS NAME EXIT_CODE STDIN MATCH_STDOUT MATCH_STDERR)
  set(MULTI_VALUE_ARGS EXTRA_ARGS MATCH_FILES PROPERTIES)
  cmake_parse_arguments(
    TEST
    "${OPTIONS}"
    "${ONE_VALUE_ARGS}"
    "${MULTI_VALUE_ARGS}"
    ${ARGN})
  # Add the test command for this executable.
  add_test_command(
    NAME ${TEST_NAME}
    COMMAND "$<TARGET_FILE:${TARGET}>" ${TEST_EXTRA_ARGS}
    EXIT_CODE ${TEST_EXIT_CODE}
    STDIN ${TEST_STDIN}
    MATCH_STDOUT ${TEST_MATCH_STDOUT}
    MATCH_STDERR ${TEST_MATCH_STDERR}
    MATCH_FILES ${TEST_MATCH_FILES}
    PROPERTIES ${TEST_PROPERTIES})
endfunction()

## Add executable that would be executed to be run during testing.
function(add_test_executable TARGET)
  # Parse and check arguments.
  if(NOT TARGET)
    message(FATAL_ERROR "Target must be specified.")
  endif()
  set(OPTIONS)
  set(ONE_VALUE_ARGS NAME EXIT_CODE STDIN MATCH_STDOUT MATCH_STDERR)
  set(MULTI_VALUE_ARGS SOURCES MATCH_FILES EXTRA_ARGS PROPERTIES)
  cmake_parse_arguments(
    TEST
    "${OPTIONS}"
    "${ONE_VALUE_ARGS}"
    "${MULTI_VALUE_ARGS}"
    ${ARGN})
  if(NOT TEST_SOURCES)
    message(FATAL_ERROR "List of sources must not be empty.")
  endif()
  # Create the executable.
  add_executable(${TARGET} ${TEST_SOURCES})
  # Add the test command for this executable.
  add_test_command_from_target(
    ${TARGET}
    NAME ${TEST_NAME}
    EXTRA_ARGS ${TEST_EXTRA_ARGS}
    EXIT_CODE ${TEST_EXIT_CODE}
    STDIN ${TEST_STDIN}
    MATCH_STDOUT ${TEST_MATCH_STDOUT}
    MATCH_STDERR ${TEST_MATCH_STDERR}
    MATCH_FILES ${TEST_MATCH_FILES}
    PROPERTIES ${TEST_PROPERTIES})
endfunction()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

# Add the actual tests.
add_subdirectory("test_runner")
add_subdirectory("tit")

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
