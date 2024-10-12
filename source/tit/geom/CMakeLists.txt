# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of the Tit Solver project, under the MIT License.
# See /LICENSE.md for license information. SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

add_tit_library(
  NAME
    geom
  SOURCES
    "bbox.hpp"
    "partition.hpp"
    "partition/recursive_bisection.hpp"
    "search.hpp"
    "search/grid_search.hpp"
    "search/kd_tree_search.hpp"
    "sort.hpp"
    "sort/morton_curve_sort.hpp"
  DEPENDS
    tit::core
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

add_tit_executable(
  NAME
    geom_tests
  SOURCES
    "bbox.test.cpp"
    "partition/recursive_bisection.test.cpp"
    "search.test.cpp"
    "sort/morton_curve_sort.test.cpp"
  DEPENDS
    tit::geom
    tit::testing
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
