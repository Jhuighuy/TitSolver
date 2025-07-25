# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
# Commercial use, including SaaS, requires a separate license, see /LICENSE.md
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

add_tit_library(
  NAME
    par
  SOURCES
    "algorithms.hpp"
    "atomic.hpp"
    "control.cpp"
    "control.hpp"
    "task_group.hpp"
  DEPENDS
    tit::core
    TBB::tbb
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

add_tit_executable(
  NAME
    par_tests
  SOURCES
    "algorithms.test.cpp"
    "atomic.test.cpp"
    "control.test.cpp"
    "task_group.test.cpp"
  DEPENDS
    tit::par
    tit::testing
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
