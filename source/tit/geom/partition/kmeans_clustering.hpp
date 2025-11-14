/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm>
#include <array>
#include <limits>
#include <random>
#include <ranges>
#include <vector>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/math.hpp"
#include "tit/core/profiler.hpp"
#include "tit/core/range.hpp"
#include "tit/core/utils.hpp"
#include "tit/geom/point_range.hpp"

namespace tit::geom {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// K-means++ clustering function.
///
/// Unlike the partitioning algorithms, K-means produces clusters of unbounded
/// size (that's why we use the term "clustering" instead of "partitioning").
class KMeansClustering final {
public:

  /// Construct a K-means clustering function.
  constexpr explicit KMeansClustering(float64_t eps = 1.0e-4,
                                      size_t max_iters = 10) noexcept
      : eps_{eps}, max_iters_{max_iters} {
    TIT_ASSERT(eps_ > 0.0, "Tolerance must be positive!");
    TIT_ASSERT(max_iters_ > 0, "Number of iterations must be positive!");
  }

  /// Partition the points using the K-means clustering algorithm.
  template<point_range Points, output_index_range Clusters>
  void operator()(Points&& points,
                  Clusters&& clusters,
                  std::ranges::range_value_t<Clusters> num_clusters,
                  std::ranges::range_value_t<Clusters> init_cluster = 0) const {
    TIT_PROFILE_SECTION("KMeansClustering::operator()");
    TIT_ASSUME_UNIVERSAL(Points, points);
    TIT_ASSUME_UNIVERSAL(Clusters, clusters);
    using Cluster = std::ranges::range_value_t<Clusters>;
    using Vec = point_range_vec_t<Points>;
    using Num = point_range_num_t<Points>;

    // Validate the arguments.
    const auto num_points = std::size(points);
    TIT_ASSERT(num_clusters > 0, "Number of clusters must be positive!");
    TIT_ASSERT(num_points >= num_clusters,
               "Number of points cannot be less than the number of clusters!");
    if constexpr (std::ranges::sized_range<Clusters>) {
      TIT_ASSERT(
          num_points == std::size(clusters),
          "Size of clusters range must be equal to the number of points!");
    }

    // Compute the initial centroids (K-means++ initialization).
    std::mt19937_64 rng{num_points};
    std::vector min_sq_dists(num_points, std::numeric_limits<Num>::max());
    std::vector<Vec> centroids(num_clusters);
    std::uniform_int_distribution points_dist(0UZ, num_points - 1);
    centroids.front() = points[points_dist(rng)];
    for (auto&& [prev_centroid, centroid] : std::views::pairwise(centroids)) {
      Num total_weight{};
      for (auto&& [point, dist_sq] : std::views::zip(points, min_sq_dists)) {
        dist_sq = std::min(dist_sq, norm2(point - prev_centroid));
        total_weight += dist_sq;
      }

      std::uniform_real_distribution weight_dist(Num{0}, total_weight);
      auto remaining_weight = weight_dist(rng);
      for (const auto& [point, dist_sq] :
           std::views::zip(points, min_sq_dists) | std::views::as_const) {
        remaining_weight -= dist_sq;
        if (remaining_weight <= Num{0}) {
          centroid = point;
          break;
        }
      }
    }

    // Sort centroids lexicographically to ensure consistent ordering.
    std::ranges::sort(centroids, {}, [](const Vec& p) { return p.elems(); });

    // Run K-means algorithm.
    std::vector<Vec> prev_centroids(num_clusters);
    std::vector<size_t> cluster_counts(num_clusters);
    for (size_t iter = 0; iter < max_iters_; ++iter) {
      // Assign points to the closest centroid.
      std::ranges::fill(cluster_counts, 0);
      for (auto&& [point, cluster] : std::views::zip(points, clusters)) {
        cluster = std::ranges::min(std::views::iota(Cluster{}, num_clusters),
                                   {},
                                   [&point, &centroids](Cluster c) {
                                     return norm2(point - centroids[c]);
                                   });
        cluster_counts[cluster]++;
      }

      // Recompute the centroids and check for convergence.
      Num delta{};
      std::swap(centroids, prev_centroids);
      std::ranges::fill(centroids, Vec{});
      for (const auto& [point, cluster] :
           std::views::zip(points, clusters) | std::views::as_const) {
        centroids[cluster] += point;
      }
      for (auto&& [prev_centroid, centroid, count] :
           std::views::zip(prev_centroids, centroids, cluster_counts)) {
        if (count == 0) {
          centroid = prev_centroid;
        } else {
          centroid /= static_cast<Num>(count);
          delta += norm2(centroid - prev_centroid);
        }
      }
      if (static_cast<float64_t>(delta) < pow2(eps_)) break;
    }

    // Assign the final cluster indices.
    for (auto& cluster : clusters) cluster += init_cluster;
  }

private:

  float64_t eps_;
  size_t max_iters_;

}; // class KMeansClustering

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::geom
