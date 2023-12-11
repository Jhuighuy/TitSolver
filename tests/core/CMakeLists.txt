# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
# Part of the Tit Solver project, under the MIT License
# See /LICENSE.md for license information.
# SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

# Specify the list of sources.
set(
  CXX_SOURCES
  "math_tests.cpp"
  "mdvector_tests.cpp"
  "string_utils_tests.cpp"
  "vec_tests.cpp")

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

# Create the doctest-based executable.
add_test_executable(tit_core_tests ${CXX_SOURCES})
add_executable(tit::core_tests ALIAS tit_core_tests)

# Link with the dependent libraries.
target_link_libraries(tit_core_tests PRIVATE tit::core)

# Enable static analysis.
check_includes(tit::core_tests)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
