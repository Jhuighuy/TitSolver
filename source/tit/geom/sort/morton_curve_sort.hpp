/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <functional>
#include <ranges>

#include "tit/core/basic_types.hpp"
#include "tit/core/par.hpp"
#include "tit/core/profiler.hpp"
#include "tit/core/utils.hpp"

#include "tit/geom/bipartition.hpp"
#include "tit/geom/utils.hpp"

namespace tit::geom {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Morton space filling curve spatial sort function.
class MortonCurveSort final {
public:

  /// Order the points along the Morton space filling curve.
  template<point_range Points, output_index_range Perm>
  void operator()(Points&& points, Perm&& perm) const {
    TIT_PROFILE_SECTION("MortonCurveSort::operator()");
    TIT_ASSUME_UNIVERSAL(Points, points);
    TIT_ASSUME_UNIVERSAL(Perm, perm);
    using Box = point_range_bbox_t<Points>;

    // Initialize sorting.
    const auto box = compute_bbox(points);
    iota_perm(points, perm);

    // Recursively partition the points along the Morton curve.
    par::TaskGroup tasks{};
    const auto impl = [&points, &tasks](this const auto& self,
                                        const Box& my_box,
                                        std::ranges::view auto my_perm,
                                        size_t axis) -> void {
      if (std::size(my_perm) <= 1) return;

      // Split the points along the current axis.
      const auto center_coord = my_box.center()[axis];
      const auto [left_box, right_box] = my_box.split(axis, center_coord);
      const auto [left_perm, right_perm] =
          coord_bisection(points, my_perm, center_coord, axis);

      // Recursively sort the parts along the next axis.
      const auto next_axis = (axis + 1) % point_range_dim_v<Points>;
      tasks.run(is_async_(left_perm),
                std::bind_front(self, left_box, left_perm, next_axis));
      tasks.run(is_async_(right_perm),
                std::bind_front(self, right_box, right_perm, next_axis));
    };
    impl(box, std::views::all(perm), /*axis=*/1);
    tasks.wait();
  }

private:

  // Should the sorting be done in parallel?
  static auto is_async_(const auto& perm) noexcept -> bool {
    constexpr size_t too_small = 50; // Empirical value.
    return std::size(perm) >= too_small;
  }

}; // class MortonCurveSort

/// Morton space filling curve spatial sort.
inline constexpr MortonCurveSort morton_curve_sort{};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::geom
