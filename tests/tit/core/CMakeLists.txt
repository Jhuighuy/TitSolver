# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
# Part of the Tit Solver project, under the MIT License.
# See /LICENSE.md for license information. SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

# Specify the list of sources.
set(
  CXX_SOURCES
  "math.cpp"
  "mdvector.cpp"
  "posix.cpp"
  "string_utils.cpp"
  "vec.cpp")

# Create the unit test executable.
add_doctest_executable(tit_core_tests ${CXX_SOURCES})
add_executable(tit::core_tests ALIAS tit_core_tests)

# Link with the dependent libraries.
target_link_libraries(tit_core_tests PRIVATE tit::core)

# Enable static analysis.
check_includes(tit::core_tests)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

# Create the test executable.
add_executable(tit_core_assert_tests "assert_tests.cpp")
add_executable(tit::core_assert_tests ALIAS tit_core_assert_tests)

# Link with the dependent libraries.
target_link_libraries(tit_core_assert_tests PRIVATE tit::core)

# Enable static analysis.
check_includes(tit::core_assert_tests)

# Add the test.
add_test_from_target(
  tit::core_assert_tests
  NAME "tit::core::assert"
  MATCH_STDERR "assert_tests_stderr.txt"
  EXIT_CODE "-1")

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
