# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
# Part of the Tit Solver project, under the MIT License
# See /LICENSE.md for license information.
# SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

# Enable CTest.
enable_testing()

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
## All the actuals tests inside of the executable are detected automatically.
## Interface of this function is pretty identical to normal `add_executable`.
function(add_test_executable TARGET)
  # Create the executable.
  add_executable(${TARGET} ${ARGN})
  # Link with doctest.
  target_link_libraries(${TARGET} PRIVATE tit::doctest)
  # Register the tests.
  doctest_discover_tests(${TARGET} EXTRA_ARGS --force-colors=true)
endfunction(add_test_executable)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

## Add executable that it to be run during testing.
function(add_custom_test_executable TARGET)
  message(FATAL_ERROR "This function is not implemented.")
  # Create the executable.
  add_executable(${TARGET} ${ARGN})
  # Specify that this executable is a test.
  add_test(NAME ${TARGET} COMMAND $<TARGET_FILE:${TARGET}>)
endfunction()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

# Add the actual tests.
add_subdirectory("core")

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
