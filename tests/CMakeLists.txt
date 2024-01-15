# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
# Part of the Tit Solver project, under the MIT License.
# See /LICENSE.md for license information. SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

include(testing)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

# Create a base library for the doctest-based test executable.
add_library(tit_doctest OBJECT "doctest_main.cpp")
add_library(tit::doctest ALIAS tit_doctest)

# Link with doctest.
target_link_libraries(tit_doctest PUBLIC doctest::doctest)

# Include doctest support module.
include(doctest)

## Add a test executable target that uses doctest as the underlying framework.
## All the actual tests inside of the executable are detected automatically.
## Interface of this function is pretty identical to normal `add_executable`.
macro(add_doctest_executable TARGET)
  # Create the executable.
  add_executable(${TARGET} ${ARGN})
  # Link with doctest.
  target_link_libraries(${TARGET} PRIVATE tit::doctest)
  # Add the tests.
  doctest_discover_tests(
    ${TARGET}
    # Use colors, since we are printing the output.
    EXTRA_ARGS --force-colors=true
    # Use the test source directory to be able to read the files.
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
endmacro()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

# Add the actual tests.
add_subdirectory("test_runner")
add_subdirectory("tit")
add_subdirectory("titwcsph")

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
