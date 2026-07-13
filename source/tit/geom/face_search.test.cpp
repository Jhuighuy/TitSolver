/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <random>
#include <ranges>
#include <vector>

#include "tit/core/vec.hpp"
#include "tit/geom/bsphere.hpp"
#include "tit/geom/face_search.hpp"
#include "tit/geom/surface.hpp"
#include "tit/geom/tessellation.hpp"
#include "tit/par/algorithms.hpp"
#include "tit/par/control.hpp"
#include "tit/testing/test.hpp"

namespace tit {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Face search result.
using SearchResult = std::vector<std::vector<std::size_t>>;

// Match the two face search results.
auto match_search_results(const SearchResult& expected_result,
                          const SearchResult& actual_result) -> bool {
  return std::ranges::equal(expected_result,
                            actual_result,
                            std::ranges::is_permutation);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Naive implementation of a face search.
template<class Vec>
auto search_naive(const geom::Surface<Vec>& surface,
                  const std::vector<Vec>& points,
                  vec_num_t<Vec> search_radius) {
  SearchResult result(points.size());
  par::set_num_threads(4);
  par::for_each(std::views::zip(points, result), [&](auto&& pair) {
    auto&& [point, result_row] = pair;
    const geom::BSphere search_sphere{point, search_radius};
    for (const auto& [face_index, face] :
         std::views::enumerate(surface.faces())) {
      if (face.intersects(search_sphere)) result_row.push_back(face_index);
    }
  });
  return result;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Face search via a grid.
template<class Vec>
auto search_grid(const geom::Surface<Vec>& surface,
                 const std::vector<Vec>& points,
                 vec_num_t<Vec> search_radius,
                 vec_num_t<Vec> size_hint) -> SearchResult {
  // Construct the grid.
  const geom::GridFaceSearch grid_face_search{size_hint};
  const auto grid_face_index = grid_face_search(surface);

  // Perform the face search.
  SearchResult result(points.size());
  par::set_num_threads(4);
  par::for_each(std::views::zip(points, result), [&](auto&& pair) {
    auto&& [point, result_row] = pair;
    grid_face_index.search(geom::BSphere{point, search_radius},
                           std::back_inserter(result_row));
  });
  return result;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Run a face search test.
template<class Vec>
void run_face_search_test(const geom::Surface<Vec>& surf,
                          const std::vector<Vec>& points,
                          vec_num_t<Vec> search_radius) {
  // Naive search as a baseline.
  INFO("Naive search");
  const auto result_naive = search_naive(surf, points, search_radius);
  REQUIRE((result_naive.empty() ||
           !std::ranges::all_of(result_naive, std::ranges::empty)));
  CHECK(match_search_results(result_naive, result_naive));

  // Face search with a grid.
  INFO("Grid search");
  for (const auto scale : {0.5, 5.0}) {
    CAPTURE(scale);
    const auto size_hint = scale * search_radius;
    const auto result_grid =
        search_grid(surf, points, search_radius, size_hint);
    CHECK(match_search_results(result_naive, result_grid));
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("geom::face_search") {
  std::mt19937 random_engine{/*seed=*/123};
  std::uniform_real_distribution dist{0.0, 1.0};
  SUBCASE("empty") {
    const std::vector<Vec<double, 2>> points;
    const geom::Surface<Vec<double, 2>> surf;
    run_face_search_test(surf, points, 0.1);
  }
  SUBCASE("2D") {
    // Generate random points in the unit square.
    std::vector<Vec<double, 2>> points(200);
    for (auto& point : points) {
      for (std::size_t i = 0; i < 2; ++i) point[i] = dist(random_engine);
    }

    // Some surface.
    geom::Surface<Vec<double, 2>> surf;
    surf.append_vert({0.0, 0.0});
    surf.append_vert({1.0, 0.0});
    surf.append_vert({1.0, 1.0});
    surf.append_vert({0.0, 1.0});
    surf.append_face({0, 1});
    surf.append_face({1, 2});
    surf.append_face({2, 3});
    surf.append_face({3, 0});
    surf = geom::tessellate(surf, 0.05);

    // Run the nearest neighbor search tests for various search radii.
    for (const auto search_radius : {0.01, 0.1, 0.5, 1.0}) {
      CAPTURE(search_radius);
      run_face_search_test(surf, points, search_radius);
    }
  }
  SUBCASE("3D") {
    // Generate random points in the unit cube.
    std::vector<Vec<double, 3>> points(200);
    for (auto& point : points) {
      for (std::size_t i = 0; i < 3; ++i) point[i] = dist(random_engine);
    }

    // Some surface.
    geom::Surface<Vec<double, 3>> surf;
    surf.append_vert({0.0, 0.0, 0.0});
    surf.append_vert({1.0, 0.0, 0.0});
    surf.append_vert({0.0, 1.0, 0.0});
    surf.append_vert({0.0, 0.0, 1.0});
    surf.append_face({0, 2, 1});
    surf.append_face({0, 1, 3});
    surf.append_face({0, 3, 2});
    surf.append_face({1, 2, 3});
    surf = geom::tessellate(surf, 0.1);

    // Run the nearest neighbor search tests for various search radii.
    for (const auto search_radius : {0.01, 0.1, 0.5, 1.0}) {
      CAPTURE(search_radius);
      run_face_search_test(surf, points, search_radius);
    }
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
