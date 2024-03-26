/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm>
#include <ranges>
#include <span>
#include <utility>
#include <vector>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/mat.hpp"
#include "tit/core/profiler.hpp"
#include "tit/core/uint_utils.hpp"
#include "tit/core/vec.hpp"

#include "tit/par/task_group.hpp"

namespace tit::geom {

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/// Inertial bisection partitioning.
template<std::ranges::view Points>
  requires std::ranges::sized_range<Points> &&
           std::ranges::random_access_range<Points> &&
           is_vec_v<std::ranges::range_value_t<Points>>
class InertialBisection final {
public:

  using Num = vec_num_t<std::ranges::range_value_t<Points>>;

  /// Numeric type used by the point type.
  static constexpr auto Dim = vec_dim_v<std::ranges::range_value_t<Points>>;

  /// Initialize and build a simple coord. bisection partitioning.
  InertialBisection(Points points, size_t num_parts)
      : points_{std::move(points)} {
    TIT_PROFILE_SECTION("InertialBisection::ctor()");
    if (std::ranges::empty(points_)) return;
    // Initialize identity point ordering.
    auto const num_points = std::ranges::size(points_);
    parts_.resize(num_points), indices_.resize(num_points);
    std::ranges::copy(std::views::iota(size_t{0}, num_points),
                      indices_.begin());
    // Build the partitioning.
    par::TaskGroup tasks{};
    partition_(tasks, indices_, num_parts, /*part_index=*/0);
  }

  void GetPartitioning(std::vector<size_t>& parts) {
    parts = std::move(parts_);
  }

private:

  void partition_(par::TaskGroup& tasks, std::span<size_t> range,
                  size_t num_parts, size_t part_index) {
    TIT_ASSERT(is_power_of_two(num_parts),
               "Number of parts must be a power of two at the moment.");
    if (num_parts == 1) {
      // No further partitioning, assign the part index to points.
      for (auto const index : range) parts_[index] = part_index;
      return;
    }
    // Compute the center of mass.
    auto center = points_[range[0]];
    for (auto i : range.subspan(1)) center += points_[i];
    center /= static_cast<Num>(range.size());
    // Compute the inertial matrix.
    auto V = outer_sqr(points_[range[0]] - center);
    for (auto i : range.subspan(1)) V += outer_sqr(points_[i] - center);
    // Compute the inertial axis.
    auto const inertial_axis = //
        jacobi(V)
            .transform([](auto const& eig) {
              // Inertial axis is the largest eigenvector.
              auto const& [Q, d] = eig;
              return Q[max_value_index(d)];
            })
            .value_or([] {
              // Use some a unit vector as an inertial axis.
              Vec<Num, Dim> axis{};
              axis[0] = Num{1.0};
              return axis;
            }());
    // Split the range by the median.
    auto const median = range.begin() + static_cast<ssize_t>(range.size() / 2);
    std::ranges::nth_element(range, median, /*cmp=*/{}, [&](size_t a) {
      return dot(inertial_axis, points_[a] - center);
    });
    // Continue partitioning.
    auto const left_range = range.subspan(0, range.size() / 2);
    tasks.run([=, &tasks, this] {
      partition_(tasks, left_range, num_parts / 2, 2 * part_index);
    });
    auto const right_range = range.subspan(range.size() / 2);
    tasks.run([=, &tasks, this] {
      partition_(tasks, right_range, num_parts / 2, 2 * part_index + 1);
    });
  }

  Points points_;
  std::vector<size_t> parts_;
  std::vector<size_t> indices_;

}; // class InertialBisection

// Wrap a viewable range into a view on construction.
template<class Points, class... Args>
InertialBisection(Points&&, Args...)
    -> InertialBisection<std::views::all_t<Points>>;

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

} // namespace tit::geom
