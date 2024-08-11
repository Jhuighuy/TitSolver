# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of the Tit Solver project, under the MIT License.
# See /LICENSE.md for license information. SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

add_tit_library(
  NAME
    geom
  SOURCES
    "bbox.hpp"
    "grid.hpp"
    "inertial_bisection.hpp"
    "kd_tree.hpp"
    "partitioning.hpp"
    "search.hpp"
    "sfc_sort.hpp"
  DEPENDS
    tit::core
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

add_tit_executable(
  NAME
    geom_tests
  SOURCES
    "bbox.test.cpp"
    "inertial_bisection.test.cpp"
    "search.test.cpp"
    "sfc_sort.test.cpp"
  DEPENDS
    tit::geom
    tit::testing
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
