/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <iterator>
#include <random>
#include <ranges>
#include <set>
#include <vector>

#include "tit/core/basic_types.hpp"
#include "tit/core/io.hpp"
#include "tit/core/math.hpp"
#include "tit/core/vec.hpp"

#include "tit/geom/kd_tree.hpp"
#include "tit/geom/knn_search.hpp"

#include "tit/testing/test.hpp"

namespace tit {
namespace {

using Vec3D = Vec<double, 3>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Generate a random double in range [0,1].
auto random_double_01() -> double {
  static std::mt19937 random_engine{123};
  std::uniform_real_distribution<double> dist(0.0, 1.0);
  return dist(random_engine);
}

// Generate a 3D vector with components in range [0,1].
auto random_vec_01() -> Vec3D {
  return Vec3D{random_double_01(), random_double_01(), random_double_01()};
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

using KNNSearchResult = std::vector<std::vector<size_t>>;

// Match the two KNN search results.
void match_search_results(const KNNSearchResult& expected,
                          const KNNSearchResult& actual) {
  REQUIRE(expected.size() == actual.size());
  for (const auto& [exp, act] : std::views::zip(expected, actual)) {
    const std::set exp_set(exp.begin(), exp.end());
    const std::set act_set(act.begin(), act.end());
    // Ensure there are no duplicates.
    CHECK(act.size() == act_set.size());
    // Ensure the sets are equal.
    CHECK(exp_set == act_set);
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Naive implementation of a KNN search.
auto knn_search_naive(const std::vector<Vec3D>& points,
                      double search_radius) -> KNNSearchResult {
  const auto search_radius_sqr = pow2(search_radius);
  KNNSearchResult result(points.size());
  for (size_t i = 0; i < points.size(); ++i) {
    for (size_t j = 0; j < points.size(); ++j) {
      if (norm2(points[i] - points[j]) < search_radius_sqr) {
        result[i].push_back(j);
      }
    }
  }
  return result;
}

// KNN search via a K-dimensional tree.
auto knn_search_kdtree(const std::vector<Vec3D>& points,
                       double search_radius) -> KNNSearchResult {
  // Construct the K-dimensional tree.
  const geom::KDTree kd_tree{points};
  // Perform the KNN search.
  KNNSearchResult result(points.size());
  for (size_t i = 0; i < points.size(); ++i) {
    kd_tree.search(points[i], search_radius, std::back_inserter(result[i]));
  }
  return result;
}

// KNN search via a grid.
auto knn_search_grid(const std::vector<Vec3D>& points,
                     double search_radius) -> KNNSearchResult {
  // Construct the K-dimensional tree.
  const geom::Grid grid{points, 0.5 * search_radius};
  // Perform the KNN search.
  KNNSearchResult result(points.size());
  for (size_t i = 0; i < points.size(); ++i) {
    grid.search(points[i], search_radius, std::back_inserter(result[i]));
  }
  return result;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("geom::KNNSearch") {
  // Generate random points.
  constexpr size_t NumPoints = 1000;
  std::vector<Vec3D> points(NumPoints);
  for (auto& point : points) point = random_vec_01();
  // Perform the KNN search using a naive approach.
  constexpr double search_radius = 0.1;
  const auto result_naive = knn_search_naive(points, search_radius);
  match_search_results(result_naive, result_naive);
  // Perform the KNN search with a K-dimensional tree.
  println("=== KNN search with a K-dimensional tree ===");
  const auto result_kdtree = knn_search_kdtree(points, search_radius);
  match_search_results(result_naive, result_kdtree);
  // Perform the KNN search with a grid.
  println("=== KNN search with a grid ===");
  const auto result_grid = knn_search_grid(points, search_radius);
  match_search_results(result_naive, result_grid);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
