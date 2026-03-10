/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <array>

#include "tit/core/basic_types.hpp"
#include "tit/core/vec.hpp"
#include "tit/geom/partition/kmeans_clustering.hpp"
#include "tit/testing/test.hpp"

namespace tit {
namespace {

using Vec2D = Vec<double, 2>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("geom::KMeansClustering") {
  SUBCASE("two clusters") {
    // Create points in two clearly separated groups.
    const std::array<Vec2D, 6> points{{
        // Group A near origin.
        {0.0, 0.0},
        {0.1, 0.0},
        {0.0, 0.1},
        // Group B far away.
        {10.0, 10.0},
        {10.1, 10.0},
        {10.0, 10.1},
    }};

    // Cluster the points into 2 clusters.
    std::array<size_t, 6> clusters{};
    geom::kmeans_clustering(points, clusters, 2);

    // All points within each group must belong to the same cluster.
    CHECK(clusters[0] == clusters[1]);
    CHECK(clusters[0] == clusters[2]);
    CHECK(clusters[3] == clusters[4]);
    CHECK(clusters[3] == clusters[5]);

    // The two groups must belong to different clusters.
    CHECK(clusters[0] != clusters[3]);
  }

  SUBCASE("four clusters") {
    // Create points in four clearly separated groups at the corners of a
    // 10x10 square.
    const std::array<Vec2D, 12> points{{
        // Group A.
        {0.0, 0.0},
        {0.1, 0.0},
        {0.0, 0.1},
        // Group B.
        {10.0, 0.0},
        {10.1, 0.0},
        {10.0, 0.1},
        // Group C.
        {0.0, 10.0},
        {0.1, 10.0},
        {0.0, 10.1},
        // Group D.
        {10.0, 10.0},
        {10.1, 10.0},
        {10.0, 10.1},
    }};

    // Cluster the points into 4 clusters.
    std::array<size_t, 12> clusters{};
    geom::kmeans_clustering(points, clusters, 4);

    // All points within each group must belong to the same cluster.
    for (size_t g = 0; g < 4; ++g) {
      CHECK(clusters[3 * g] == clusters[3 * g + 1]);
      CHECK(clusters[3 * g] == clusters[3 * g + 2]);
    }

    // All four groups must belong to different clusters.
    CHECK(clusters[0] != clusters[3]);
    CHECK(clusters[0] != clusters[6]);
    CHECK(clusters[0] != clusters[9]);
    CHECK(clusters[3] != clusters[6]);
    CHECK(clusters[3] != clusters[9]);
    CHECK(clusters[6] != clusters[9]);
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
