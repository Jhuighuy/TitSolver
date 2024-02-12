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
#include "tit/core/profiler.hpp"
#include "tit/core/uint_utils.hpp"
#include "tit/core/vec.hpp"

#include "tit/par/task_group.hpp"

namespace tit::geom {

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/// Simple recursive coordinate bisection partitioning.
template<std::ranges::view Points>
  requires std::ranges::sized_range<Points> &&
           std::ranges::random_access_range<Points> &&
           is_vec_v<std::ranges::range_value_t<Points>>
class SimpleCoordinateBisection final {
public:

  /// Numeric type used by the point type.
  static constexpr auto Dim = vec_dim_v<std::ranges::range_value_t<Points>>;

  /// Initialize and build a simple coord. bisection partitioning.
  SimpleCoordinateBisection(Points points, size_t num_parts, size_t axis = 0)
      : points_{std::move(points)}, axis_{axis} {
    TIT_PROFILE_SECTION("SimpleCoordinateBisection::ctor()");
    TIT_ASSERT(axis_ < Dim, "Axis is invalid!");
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
    // Split the range by the median.
    auto const median = range.begin() + static_cast<ssize_t>(range.size() / 2);
    std::ranges::nth_element(range, median, /*cmp=*/{},
                             [&](size_t a) { return points_[a][0]; });
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
  size_t axis_;
  std::vector<size_t> parts_;
  std::vector<size_t> indices_;

}; // class SimpleCoordinateBisection

// Wrap a viewable range into a view on construction.
template<class Points, class... Args>
SimpleCoordinateBisection(Points&&, Args...)
    -> SimpleCoordinateBisection<std::views::all_t<Points>>;

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

} // namespace tit::geom
