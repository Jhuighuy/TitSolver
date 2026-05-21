# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of BlueTit Solver, under the MIT License.
# See /LICENSE.md for license information. SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

add_tit_library(
  NAME
    geom
  SOURCES
    "bbox.hpp"
    "bipartition.hpp"
    "bsphere.hpp"
    "face_search.hpp"
    "face_search/grid_face_search.hpp"
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
    "segment.hpp"
    "sort.hpp"
    "sort/hilbert_curve_sort.hpp"
    "sort/morton_curve_sort.hpp"
    "surface.hpp"
    "triangle.hpp"
  DEPENDS
    tit::core
    tit::par
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

add_tit_executable(
  NAME
    geom_tests
  SOURCES
    "bbox.test.cpp"
    "bipartition.test.cpp"
    "bsphere.test.cpp"
    "face_search.test.cpp"
    "grid.test.cpp"
    "partition/kmeans_clustering.test.cpp"
    "partition/pixelated_partition.test.cpp"
    "partition/recursive_bisection.test.cpp"
    "partition/sort_partition.test.cpp"
    "point_range.test.cpp"
    "search.test.cpp"
    "segment.test.cpp"
    "sort/hilbert_curve_sort.test.cpp"
    "sort/morton_curve_sort.test.cpp"
    "surface.test.cpp"
    "triangle.test.cpp"
  DEPENDS
    tit::geom
    tit::testing
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
