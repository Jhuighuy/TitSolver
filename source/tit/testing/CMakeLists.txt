# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
# Commercial use, including SaaS, requires a separate license, see /LICENSE.md
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
  DEPENDS
    tit::core
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
