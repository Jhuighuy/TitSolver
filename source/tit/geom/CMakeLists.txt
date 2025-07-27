# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
# Commercial use, including SaaS, requires a separate license, see /LICENSE.md
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

add_tit_library(
  NAME
    geom
  SOURCES
    "bbox.hpp"
    "bipartition.hpp"
    "grid.hpp"
    "partition.hpp"
    "partition/kmeans_clustering.hpp"
    "partition/pixelated_partition.hpp"
    "partition/recursive_bisection.hpp"
    "partition/sort_partition.hpp"
    "point_range.hpp"
    "search.hpp"
    "search/grid_search.hpp"
    "search/kd_tree_search.hpp"
    "sort.hpp"
    "sort/hilbert_curve_sort.hpp"
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
    "bipartition.test.cpp"
    "grid.test.cpp"
    "point_range.test.cpp"
    "partition/recursive_bisection.test.cpp"
    "partition/sort_partition.test.cpp"
    "search.test.cpp"
    "sort/hilbert_curve_sort.test.cpp"
    "sort/morton_curve_sort.test.cpp"
  DEPENDS
    tit::geom
    tit::testing
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
