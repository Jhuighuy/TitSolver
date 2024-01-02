# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
# Part of the Tit Solver project, under the MIT License.
# See /LICENSE.md for license information. SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

# Specify the list of sources.
set(
  CXX_SOURCES
  "math.cpp"
  "mdvector.cpp"
  "meta.cpp"
  "signals.cpp"
  "string_utils.cpp"
  "vec.cpp")

# Create the unit test executable.
add_executable(tit_core_tests ${CXX_SOURCES})
add_executable(tit::core_tests ALIAS tit_core_tests)

# Link with the dependent libraries.
target_link_libraries(tit_core_tests PRIVATE tit::core tit::doctest)

# Enable static analysis.
enable_static_analysis(tit::core_tests)

# Register the test.
add_test_from_target(tit::core_tests NAME "tit/core/unit")

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
