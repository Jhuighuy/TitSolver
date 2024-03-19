# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
# Part of the Tit Solver project, under the MIT License.
# See /LICENSE.md for license information. SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

set(
  SOURCES
  test.hpp
  test_main.cpp)

# Create a library. It must be an object library in order to make the test
# registration work properly.
add_library(tit_testing OBJECT ${SOURCES})
add_library(tit::testing ALIAS tit_testing)

# Link with doctest.
target_link_libraries(tit_testing PUBLIC tit::base doctest::doctest)

# Enable static analysis.
enable_clang_tidy(tit::testing)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
