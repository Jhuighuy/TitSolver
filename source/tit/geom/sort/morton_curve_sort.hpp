/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <functional>
#include <numeric>
#include <ranges>
#include <span>

#include "tit/core/basic_types.hpp"
#include "tit/core/profiler.hpp"
#include "tit/geom/bipartition.hpp"
#include "tit/geom/point_range.hpp"
#include "tit/par/task_group.hpp"

namespace tit::geom {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Morton space filling curve spatial sort function.
class MortonCurveSort final {
public:

  /// Order the points along the Morton space filling curve.
  template<point_range Points>
  void operator()(Points&& points, std::span<size_t> perm) const {
    TIT_PROFILE_SECTION("MortonCurveSort::operator()");
    using Box = point_range_bbox_t<Points>;

    // Initialize sorting.
    const auto box = compute_bbox(points);
    std::ranges::iota(perm, size_t{0});

    // Recursively partition the points along the Morton curve.
    // Initial axis is set to Y to match the classic Morton curve definition.
    par::TaskGroup tasks{};
    const auto impl = [&points, &tasks](this const auto& self,
                                        const Box& my_box,
                                        std::ranges::view auto my_perm,
                                        size_t axis) -> void {
      if (std::ranges::size(my_perm) <= 1) return;

      // Split the points along the current axis.
      const auto center_coord = my_box.center()[axis];
      const auto [left_box, right_box] = my_box.split(axis, center_coord);
      const auto [left_perm, right_perm] =
          coord_bisection(points, my_perm, center_coord, axis);

      // Recursively sort the parts along the next axis.
      const auto next_axis = (axis + 1) % point_range_dim_v<Points>;
      constexpr size_t min_par_size = 50;
      using enum par::RunMode;
      tasks.run(std::bind_front(self, left_box, left_perm, next_axis),
                std::ranges::size(left_perm) >= min_par_size ? parallel :
                                                               sequential);
      tasks.run(std::bind_front(self, right_box, right_perm, next_axis),
                std::ranges::size(right_perm) >= min_par_size ? parallel :
                                                                sequential);
    };
    impl(box, std::views::all(perm), /*axis=*/1);
    tasks.wait();
  }

}; // class MortonCurveSort

/// Morton space filling curve spatial sort.
inline constexpr MortonCurveSort morton_curve_sort{};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::geom
