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

#include "tit/core/math.hpp"
#include "tit/core/vec.hpp"
#include "tit/geom/bsphere.hpp"
#include "tit/geom/search.hpp"
#include "tit/par/algorithms.hpp"
#include "tit/par/control.hpp"
#include "tit/testing/test.hpp"

namespace tit {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Nearest neighbor search result.
using SearchResult = std::vector<std::vector<std::size_t>>;

// Match the two nearest neighbor search results.
auto match_search_results(const SearchResult& expected_result,
                          const SearchResult& actual_result) -> bool {
  return std::ranges::equal(expected_result,
                            actual_result,
                            std::ranges::is_permutation);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Naive O(N^2) implementation of a nearest neighbor search.
template<class Vec>
auto search_naive(const std::vector<Vec>& points, vec_num_t<Vec> search_radius)
    -> SearchResult {
  const auto search_radius_sq = pow2(search_radius);
  SearchResult result(points.size());
  for (std::size_t i = 0; i < points.size(); ++i) {
    result[i] = {i};
    for (std::size_t j = 0; j < i; ++j) {
      if (norm2(points[i] - points[j]) < search_radius_sq) {
        result[i].push_back(j);
        result[j].push_back(i);
      }
    }
  }
  return result;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Nearest neighbor search via a grid.
template<class Vec>
auto search_grid(const std::vector<Vec>& points,
                 vec_num_t<Vec> search_radius,
                 vec_num_t<Vec> size_hint) -> SearchResult {
  // Construct the grid.
  const geom::GridSearch grid_search{size_hint};
  const auto grid_index = grid_search(points);

  // Perform the nearest neighbor search.
  SearchResult result(points.size());
  par::set_num_threads(4);
  par::for_each(std::views::zip(points, result), [&](auto&& pair) {
    auto&& [point, result_row] = pair;
    grid_index.search(geom::BSphere{point, search_radius},
                      std::back_inserter(result_row));
  });
  return result;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Nearest neighbor search via a K-dimensional tree.
template<class Vec>
auto search_kd_tree(const std::vector<Vec>& points,
                    vec_num_t<Vec> search_radius) -> SearchResult {
  // Construct the K-dimensional tree.
  const geom::KDTreeSearch kd_tree_search{};
  const auto kd_tree_index = kd_tree_search(points);

  // Perform the nearest neighbor search.
  SearchResult result(points.size());
  par::set_num_threads(4);
  par::for_each(std::views::zip(points, result), [&](auto&& pair) {
    auto&& [point, result_row] = pair;
    kd_tree_index.search(geom::BSphere{point, search_radius},
                         std::back_inserter(result_row));
  });
  return result;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Run a search test.
template<class Vec>
void run_search_test(const std::vector<Vec>& points,
                     vec_num_t<Vec> search_radius) {
  // Naive search as a baseline.
  INFO("Naive search");
  const auto result_naive = search_naive(points, search_radius);
  REQUIRE((result_naive.empty() ||
           !std::ranges::all_of(result_naive, std::ranges::empty)));
  CHECK(match_search_results(result_naive, result_naive));

  // Nearest neighbor search with a grid.
  INFO("Grid search");
  for (const auto scale : {0.5, 5.0}) {
    CAPTURE(scale);
    const auto size_hint = scale * search_radius;
    const auto result_grid = search_grid(points, search_radius, size_hint);
    CHECK(match_search_results(result_naive, result_grid));
  }

  // Nearest neighbor search with a K-dimensional tree.
  INFO("KD tree search");
  const auto result_kd_tree = search_kd_tree(points, search_radius);
  CHECK(match_search_results(result_naive, result_kd_tree));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("geom::search") {
  std::mt19937 random_engine{/*seed=*/123};
  std::uniform_real_distribution dist{0.0, 1.0};
  SUBCASE("empty") {
    const std::vector<Vec<double, 2>> points;
    run_search_test(points, 0.1);
  }
  SUBCASE("2D") {
    // Generate random points in the unit square.
    std::vector<Vec<double, 2>> points(200);
    for (auto& point : points) {
      for (std::size_t i = 0; i < 2; ++i) point[i] = dist(random_engine);
    }

    // Run the nearest neighbor search tests for various search radii.
    for (const auto search_radius : {0.01, 0.1, 0.5, 1.0}) {
      CAPTURE(search_radius);
      run_search_test(points, search_radius);
    }
  }
  SUBCASE("3D") {
    // Generate random points in the unit cube.
    std::vector<Vec<double, 3>> points(200);
    for (auto& point : points) {
      for (std::size_t i = 0; i < 3; ++i) point[i] = dist(random_engine);
    }

    // Run the nearest neighbor search tests for various search radii.
    for (const auto search_radius : {0.01, 0.1, 0.5, 1.0}) {
      CAPTURE(search_radius);
      run_search_test(points, search_radius);
    }
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
