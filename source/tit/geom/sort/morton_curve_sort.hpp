/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm>
#include <ranges>

#include "tit/core/basic_types.hpp"
#include "tit/core/par.hpp"
#include "tit/core/profiler.hpp"
#include "tit/core/type_traits.hpp"
#include "tit/core/utils.hpp"

#include "tit/geom/bbox.hpp"
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
    static constexpr auto Dim = point_range_dim_v<Points>;

    // Initialize sorting.
    const auto box = compute_bbox(points);
    std::ranges::copy(std::views::iota(0UZ, std::size(perm)), std::begin(perm));

    // Recursively partition the points along the Morton curve.
    par::TaskGroup tasks{};
    const auto impl = [&points, &tasks]<size_t Axis>( //
                          this const auto& self,
                          const Box& my_box,
                          std::ranges::view auto my_perm,
                          value_constant_t<Axis> /*axis*/) {
      if (std::size(my_perm) <= 1) return;

      // Split permutation along the current axis.
      const auto center_coord = my_box.center()[Axis];
      const auto [left_box, right_box] = my_box.split(Axis, center_coord);
      const auto [left_perm, right_perm] =
          coord_bisection(points, my_perm, center_coord, Axis);

      // Recursively sort the parts along the next axis.
      static constexpr auto NextAxis = (Axis + 1) % Dim;
      tasks.run(is_async_(left_perm), [left_box, left_perm, &self] {
        self(left_box, left_perm, value_constant_t<NextAxis>{});
      });
      tasks.run(is_async_(right_perm), [right_box, right_perm, &self] {
        self(right_box, right_perm, value_constant_t<NextAxis>{});
      });
    };
    impl(box, std::views::all(perm), /*axis=*/value_constant_t<1UZ>{});
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
