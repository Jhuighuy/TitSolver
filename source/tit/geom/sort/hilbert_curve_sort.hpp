/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm>
#include <array>
#include <ranges>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/par.hpp"
#include "tit/core/profiler.hpp"
#include "tit/core/type_traits.hpp"
#include "tit/core/utils.hpp"

#include "tit/geom/bbox.hpp"
#include "tit/geom/bipartition.hpp"
#include "tit/geom/utils.hpp"

namespace tit::geom {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Hilbert space filling curve spatial sort function.
class HilbertCurveSort final {
public:

  /// Order the points along the Hilbert space filling curve.
  template<point_range Points, output_index_range Perm>
  void operator()(Points&& points, Perm&& perm) const {
    TIT_PROFILE_SECTION("HilbertCurveSort::operator()");
    TIT_ASSUME_UNIVERSAL(Points, points);
    TIT_ASSUME_UNIVERSAL(Perm, perm);
    using Box = point_range_bbox_t<Points>;
    static constexpr auto Dim = point_range_dim_v<Points>;
    using CurveState = State_<Dim>;

    // Initialize sorting.
    const auto box = compute_bbox(points);
    std::ranges::copy(std::views::iota(0UZ, std::size(perm)), std::begin(perm));

    // Recursively partition the points along the Hilbert curve.
    par::TaskGroup tasks{};
    const auto impl = [&points, &tasks]< //
                          CurveState State,
                          size_t Depth = 0,
                          size_t Index = 0>(
                          this const auto& self,
                          const Box& my_box,
                          std::ranges::view auto my_perm,
                          value_constant_t<State> /*state*/,
                          value_constant_t<Depth> /*depth*/ = {},
                          value_constant_t<Index> /*index*/ = {}) {
      if (std::size(my_perm) <= 1) return;

      // Determine the current axis and direction.
      constexpr auto Axis = State.axis;
      constexpr auto Dirs = State.dirs;
      constexpr auto CurrAxis = (Axis + Depth) % Dim;
      constexpr auto CurrDir = (Index % 2 == 0) ? Dirs[Depth] : !Dirs[Depth];

      // Split permutation along the current axis.
      const auto center_coord = my_box.center()[CurrAxis];
      const auto [left_box, right_box] =
          my_box.split(CurrAxis, center_coord, CurrDir);
      const auto [left_perm, right_perm] =
          coord_bisection(points, my_perm, center_coord, CurrAxis, CurrDir);

      if constexpr (Depth < Dim - 1) {
        // Continue partitioning within the current state.
        tasks.run(is_async_(left_perm), [left_box, left_perm, &self] {
          self(left_box,
               left_perm,
               value_constant_t<State>{},
               value_constant_t<Depth + 1>{},
               value_constant_t<2 * Index>{});
        });
        tasks.run(is_async_(right_perm), [right_box, right_perm, &self] {
          self(right_box,
               right_perm,
               value_constant_t<State>{},
               value_constant_t<Depth + 1>{},
               value_constant_t<2 * Index + 1>{});
        });
      } else {
        // Recure into the next state.
        tasks.run(is_async_(left_perm), [left_box, left_perm, &self] {
          static constexpr auto NextState = next_state_(State, 2 * Index);
          self(left_box, left_perm, value_constant_t<NextState>{});
        });
        tasks.run(is_async_(right_perm), [right_box, right_perm, &self] {
          static constexpr auto NextState = next_state_(State, 2 * Index + 1);
          self(right_box, right_perm, value_constant_t<NextState>{});
        });
      }
    };
    impl(box, std::views::all(perm), value_constant_t<CurveState{}>{});
    tasks.wait();
  }

private:

  // Hilbert curve state type.
  template<size_t Dim>
  struct State_ {
    size_t axis;
    std::array<bool, Dim> dirs;
  };

  // Compute the next state.
  template<size_t Dim>
  static consteval auto next_state_(const State_<Dim>& state,
                                    size_t index) noexcept -> State_<Dim> {
    const auto& [axis, dirs] = state;
    if constexpr (Dim == 1) {
      return {axis, dirs};
    } else if constexpr (Dim == 2) {
      const std::array axes{axis, (axis + 1) % Dim};
      switch (index) {
        case 0:  return {axes[1], {dirs[1], dirs[0]}};
        case 1:
        case 2:  return {axes[0], {dirs[0], dirs[1]}};
        case 3:  return {axes[1], {!dirs[1], !dirs[0]}};
        default: TIT_ASSERT(false, "Index is out of range!");
      }
    } else if constexpr (Dim == 3) {
      const std::array axes{axis, (axis + 1) % Dim, (axis + 2) % Dim};
      switch (index) {
        case 0:  return {axes[2], {dirs[2], dirs[0], dirs[1]}};
        case 1:
        case 2:  return {axes[1], {dirs[1], dirs[2], dirs[0]}};
        case 3:
        case 4:  return {axes[0], {dirs[0], !dirs[1], !dirs[2]}};
        case 5:
        case 6:  return {axes[1], {!dirs[1], dirs[2], !dirs[0]}};
        case 7:  return {axes[2], {!dirs[2], !dirs[0], dirs[1]}};
        default: TIT_ASSERT(false, "Index is out of range!");
      }
    } else static_assert(false);
    return {};
  }

  // Should the sorting be done in parallel?
  static auto is_async_(const auto& perm) noexcept -> bool {
    constexpr size_t too_small = 50; // Empirical value.
    return std::size(perm) >= too_small;
  }

}; // class HilbertCurveSort

/// Hilbert space filling curve spatial sort.
inline constexpr HilbertCurveSort hilbert_curve_sort{};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::geom
