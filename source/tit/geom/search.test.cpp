/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <iterator>
#include <random>
#include <ranges>
#include <set>
#include <vector>

#include "tit/core/basic_types.hpp"
#include "tit/core/math.hpp"
#include "tit/core/vec.hpp"

#include "tit/geom/search.hpp"

#include "tit/testing/test.hpp"

namespace tit {
namespace {

using Vec3D = Vec<double, 3>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Nearest neighbor search result.
using SearchResult = std::vector<std::vector<size_t>>;

// Match the two nearest neighbor search results.
void match_search_results(const SearchResult& expected_result,
                          const SearchResult& actual_result) {
  REQUIRE(expected_result.size() == actual_result.size());
  for (const auto& [expected_row, actual_row] :
       std::views::zip(expected_result, actual_result)) {
    const auto expected_set = expected_row | std::ranges::to<std::set>();
    const auto actual_set = actual_row | std::ranges::to<std::set>();
    CHECK(expected_set == actual_set);
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Naive O(N^2) implementation of a nearest neighbor search.
auto search_naive(const std::vector<Vec3D>& points, double search_radius)
    -> SearchResult {
  const auto search_radius_sqr = pow2(search_radius);
  SearchResult result(points.size());
  for (size_t i = 0; i < points.size(); ++i) {
    result[i] = {i};
    for (size_t j = 0; j < i; ++j) {
      if (norm2(points[i] - points[j]) < search_radius_sqr) {
        result[i].push_back(j);
        result[j].push_back(i);
      }
    }
  }
  return result;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Nearest neighbor search via a grid.
auto search_grid(const std::vector<Vec3D>& points,
                 double search_radius,
                 double size_hint) -> SearchResult {
  // Construct the grid.
  const geom::GridSearch grid_search{size_hint};
  const auto grid_index = grid_search(points);

  // Perform the nearest neighbor search.
  SearchResult result(points.size());
  for (const auto& [point, result_row] : std::views::zip(points, result)) {
    grid_index.search(point, search_radius, std::back_inserter(result_row));
  }
  return result;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Nearest neighbor search via a K-dimensional tree.
auto search_kd_tree(const std::vector<Vec3D>& points,
                    double search_radius,
                    size_t max_leaf_size) -> SearchResult {
  // Construct the K-dimensional tree.
  const geom::KDTreeSearch kd_tree_search{max_leaf_size};
  const auto kd_tree_index = kd_tree_search(points);

  // Perform the nearest neighbor search.
  SearchResult result(points.size());
  for (const auto& [point, result_row] : std::views::zip(points, result)) {
    kd_tree_index.search(point, search_radius, std::back_inserter(result_row));
  }
  return result;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("geom::Search") {
  // Generate random points in the unit cube.
  static const auto points = [] {
    std::mt19937 random_engine{/*seed=*/123};
    std::uniform_real_distribution<double> dist{0.0, 1.0};
    std::vector<Vec3D> points_(1000);
    for (auto& point : points_) {
      for (size_t i = 0; i < 3; ++i) point[i] = dist(random_engine);
    }
    return points_;
  }();

  // Nearest neighbor search using a naive approach.
  constexpr double search_radius = 0.1;
  static const auto result_naive = search_naive(points, search_radius);
  SUBCASE("naive") {
    match_search_results(result_naive, result_naive);
  }

  // Nearest neighbor search with a grid.
  SUBCASE("grid") {
    SUBCASE("size hint = 0.5 * search radius") {
      const auto result_grid =
          search_grid(points, search_radius, 0.5 * search_radius);
      match_search_results(result_naive, result_grid);
    }
    SUBCASE("size hint = 5.0 * search radius") {
      const auto result_grid =
          search_grid(points, search_radius, 5.0 * search_radius);
      match_search_results(result_naive, result_grid);
    }
  }

  // Nearest neighbor search with a K-dimensional tree.
  SUBCASE("KD tree") {
    SUBCASE("max leaf size = 1") {
      const auto result_kd_tree = search_kd_tree(points, search_radius, 1);
      match_search_results(result_naive, result_kd_tree);
    }
    SUBCASE("max leaf size = 10") {
      const auto result_kd_tree = search_kd_tree(points, search_radius, 10);
      match_search_results(result_naive, result_kd_tree);
    }
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
