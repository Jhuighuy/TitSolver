# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
# Commercial use, including SaaS, requires a separate license, see /LICENSE.md
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

add_tit_library(
  NAME
    graph
  SOURCES
    "coarsen.hpp"
    "graph.hpp"
    "partition.hpp"
    "partition/greedy.hpp"
    "partition/multilevel.hpp"
    "partition/simple.hpp"
    "refine.hpp"
    "utils.hpp"
  DEPENDS
    tit::core
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

add_tit_executable(
  NAME
    graph_tests
  SOURCES
    "coarsen.test.cpp"
    "graph.test.cpp"
    "partition/greedy.test.cpp"
    "partition/multilevel.test.cpp"
    "partition/simple.test.cpp"
    "refine.test.cpp"
    "utils.test.cpp"
  DEPENDS
    tit::graph
    tit::testing
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
