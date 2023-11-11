# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
# Part of the Tit Solver project, under the MIT License
# See /LICENSE.md for license information.
# SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

# Specify the list of sources.
file(GLOB_RECURSE CXX_SOURCES "*.cpp")

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

# Create the execitable.
add_executable(tit_unit_tests ${CXX_SOURCES})
add_executable(tit::unit_tests ALIAS tit_unit_tests)

# Link with the dependant libraries.
target_link_libraries(tit_unit_tests PRIVATE tit::core)

# Find doctest.
find_package(doctest CONFIG REQUIRED)
target_link_libraries(tit_unit_tests PRIVATE doctest::doctest)

# Enable some static analysis.
check_includes(tit::unit_tests)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
