# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of BlueTit Solver, under the MIT License.
# See /LICENSE.md for license information. SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

add_tit_library(
  NAME
    testing
  TYPE
    OBJECT
  SOURCES
    "math/integrals.hpp"
    "numbers/dual.hpp"
    "numbers/tagged.hpp"
    "test_main.cpp"
    "test.hpp"
    "utils.hpp"
  DEPENDS
    tit::core
    doctest::doctest
)

# Variant of the test entry point for the MPI-enabled test executables.
# Test executables should link either against `tit::testing`, or against
# `tit::testing_mpi`, never both.
add_tit_library(
  NAME
    testing_mpi
  TYPE
    OBJECT
  SOURCES
    "test_main_mpi.cpp"
  DEPENDS
    tit::core
    tit::mpi
    doctest::doctest
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

add_tit_executable(
  NAME
    testing_tests
  SOURCES
    "math/integrals.test.cpp"
    "numbers/dual.test.cpp"
  DEPENDS
    tit::core
    tit::geom
    tit::testing
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
