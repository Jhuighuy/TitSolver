# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of BlueTit Solver, under the MIT License.
# See /LICENSE.md for license information. SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

add_tit_library(
  NAME
    sym
  SOURCES
    "expression.cpp"
    "expression.hpp"
    "optimize.cpp"
    "optimize.hpp"
    "polynomial.cpp"
    "polynomial.hpp"
    "rational.cpp"
    "rational.hpp"
    "rewrite.cpp"
    "rewrite.hpp"
  DEPENDS
    tit::core
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

add_tit_executable(
  NAME
    sym_tests
  SOURCES
    "expression.test.cpp"
    "optimize.test.cpp"
    "polynomial.test.cpp"
    "rational.test.cpp"
    "rewrite.test.cpp"
  DEPENDS
    tit::sym
    tit::testing
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
