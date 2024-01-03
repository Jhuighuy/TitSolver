# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
# Part of the Tit Solver project, under the MIT License.
# See /LICENSE.md for license information. SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

# Specify the list of sources.
set(
  CXX_SOURCES
  "memory_pool.cpp")

# Create the unit test executable.
add_executable(tit_par_tests ${CXX_SOURCES})
add_executable(tit::par_tests ALIAS tit_par_tests)

# Link with the dependent libraries.
target_link_libraries(tit_par_tests PRIVATE tit::par tit::doctest)

# Enable static analysis.
enable_static_analysis(tit::par_tests)

# Register the test.
add_test_from_target(tit::par_tests NAME "tit/par/unit")

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
