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
#include "tit/core/par.hpp"
#include "tit/core/profiler.hpp"
#include "tit/core/vec.hpp"

#include "tit/geom/bbox.hpp"

namespace tit::geom {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Inertial bisection partitioning.
template<std::ranges::view Points, std::ranges::view Parts>
  requires (std::ranges::sized_range<Points> &&
            std::ranges::random_access_range<Points> &&
            is_vec_v<std::ranges::range_value_t<Points>>) &&
           (std::ranges::random_access_range<Parts> &&
            std::ranges::output_range<Parts, size_t>)
class CoordinateBisection final {
public:

  /// Initialize and build the partitioning.
  CoordinateBisection(Points points,
                      Parts parts,
                      size_t num_parts,
                      size_t init_part = 0)
      : points_{std::move(points)}, parts_{std::move(parts)} {
    TIT_PROFILE_SECTION("CoordinateBisection::CoordinateBisection()");
    build_(num_parts, init_part);
  }

  /// Get the part range.
  constexpr auto part(size_t part_index) const noexcept
      -> std::span<const size_t> {
    TIT_ASSERT(part_index < parts_ranges_.size(),
               "Part index is out of range!");
    return parts_ranges_[part_index];
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

private:

  // Build the partitioning recursively.
  void build_(size_t num_parts, size_t init_part) {
    // Initialize identity permutation.
    parts_ranges_.resize(init_part + num_parts);
    perm_.resize(std::size(points_));
    std::ranges::copy(std::views::iota(size_t{0}, perm_.size()), perm_.begin());

    // Build the partitioning.
    par::TaskGroup tasks{};
    partition_(tasks, num_parts, init_part, perm_);
    tasks.wait();
  }

  // Partition the points by bisecting the longest inertial axis.
  void partition_(par::TaskGroup& tasks,
                  size_t num_parts,
                  size_t part_index,
                  std::span<size_t> perm) {
    if (num_parts == 1) {
      // No further partitioning, assign part index to the output.
      std::ranges::sort(perm);
      parts_ranges_[part_index] = perm;
      for (const auto i : perm) parts_[i] = part_index;
      return;
    }

    BBox box{points_[perm.front()]};
    for (const auto i : perm | std::views::drop(1)) box.expand(points_[i]);
    const auto cut_dim = max_value_index(box.extents());

    // Split the parts into halves.
    const auto left_num_parts = num_parts / 2;
    const auto right_num_parts = num_parts - left_num_parts;
    const auto left_part_index = part_index;
    const auto right_part_index = part_index + left_num_parts;

    // Partition the permutation along the inertial axis.
    const auto median = left_num_parts * perm.size() / num_parts;
    std::ranges::nth_element( //
        perm,
        perm.begin() + static_cast<ssize_t>(median),
        [cut_dim, this](size_t i, size_t j) {
          return points_[i][cut_dim] - points_[j][cut_dim] < 0;
        });
    const auto left_range = perm.subspan(0, median);
    const auto right_range = perm.subspan(median);

    // Recursively partition the range.
    tasks.run([left_num_parts, left_part_index, left_range, &tasks, this] {
      partition_(tasks, left_num_parts, left_part_index, left_range);
    });
    tasks.run([right_range, right_num_parts, right_part_index, &tasks, this] {
      partition_(tasks, right_num_parts, right_part_index, right_range);
    });
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  Points points_;
  Parts parts_;
  std::vector<size_t> perm_;
  std::vector<std::span<const size_t>> parts_ranges_;

}; // class CoordinateBisection

// Wrap a viewable range into a view on construction.
template<class Points, class Parts, class... Args>
CoordinateBisection(Points&&, Parts&&, Args...)
    -> CoordinateBisection<std::views::all_t<Points>, std::views::all_t<Parts>>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::geom
