/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <array>
#include <ranges>
#include <utility>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/profiler.hpp"
#include "tit/core/range.hpp"
#include "tit/core/tuple.hpp"
#include "tit/core/utils.hpp"
#include "tit/geom/bipartition.hpp"
#include "tit/geom/point_range.hpp"
#include "tit/par/task_group.hpp"

namespace tit::geom {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

namespace impl {

// Hilbert sorting rotation state.
template<size_t Dim>
class HilbertRotation final {
public:

  // Construct a rotation state.
  constexpr HilbertRotation() = default;
  constexpr HilbertRotation(size_t axis, int dirs) noexcept
      : axis_{axis}, dirs_{dirs} {}

  // Get the current axis.
  constexpr auto axis() const noexcept -> size_t {
    return axis_;
  }

  // Get the current direction.
  constexpr auto dir() const noexcept -> bool {
    return (dirs_ & (1 << axis_)) != 0;
  }

  // Shift the current axis.
  constexpr auto shift() const noexcept -> HilbertRotation {
    return {(axis_ + 1) % Dim, dirs_};
  }

  // Flip the current direction.
  constexpr auto flip() const noexcept -> HilbertRotation {
    return {axis_, dirs_ ^ (1 << axis_)};
  }

  // Compute the next rotation state.
  constexpr auto next(size_t index) const noexcept -> HilbertRotation {
    TIT_ASSERT(index < (1U << Dim), "Index is out of range!");
    const auto [shift, flip] = [index] -> std::pair<size_t, int> {
      if constexpr (Dim == 1) {
        return {0, 0};
      } else if constexpr (Dim == 2) {
        constexpr std::array shifts{1, 0, 0, 1};
        constexpr std::array flips{0, 0, 0, 0b11};
        return {shifts[index], flips[index]};
      } else if constexpr (Dim == 3) {
        constexpr std::array shifts{2, 1, 1, 0, 0, 1, 1, 2};
        constexpr std::array flips{0, 0, 0, 0b110, 0b110, 0b011, 0b011, 0b101};
        return {shifts[index], flips[index]};
      } else {
        static_assert(false);
      }
    }();
    return {(axis_ + shift) % Dim, dirs_ ^ flip};
  }

  // Compute the index of the current rotation on the lowest level of recursion.
  constexpr auto index(const HilbertRotation& init) const noexcept -> size_t {
    TIT_ASSERT(axis_ == init.axis_, "Axis mismatch!");
    const auto flips = dirs_ ^ init.dirs_;
    size_t dist = 0;
    for (size_t i = 0; i < Dim; ++i) {
      const auto axis = (axis_ + i) % Dim;
      const auto flipped = (flips & (1 << axis)) >> axis;
      dist |= flipped << (Dim - i - 1);
    }
    return dist;
  }

private:

  size_t axis_ = 0;
  int dirs_ = 0;

}; // class HilbertRotation

// Hilbert sorting state.
template<size_t Dim>
class HilbertState final {
public:

  // Construct a state.
  constexpr HilbertState() = default;
  constexpr explicit HilbertState(HilbertRotation<Dim> rot) noexcept
      : init_rot_{rot}, curr_rot_{rot} {}
  constexpr explicit HilbertState(HilbertRotation<Dim> init_rot,
                                  HilbertRotation<Dim> curr_rot) noexcept
      : init_rot_{init_rot}, curr_rot_{curr_rot} {}

  // Get the current axis.
  constexpr auto axis() const noexcept -> size_t {
    return curr_rot_.axis();
  }

  // Get the current direction.
  constexpr auto dir() const noexcept -> bool {
    return curr_rot_.dir();
  }

  // Compute a pair of next states.
  constexpr auto next() const noexcept -> pair_of_t<HilbertState> {
    const auto next_rot = curr_rot_.shift();
    if (next_rot.axis() != init_rot_.axis()) {
      // Rotate the current state.
      return {HilbertState{init_rot_, next_rot},
              HilbertState{init_rot_, next_rot.flip()}};
    }
    // Advance to the next state.
    const auto index = next_rot.index(init_rot_);
    return {HilbertState{init_rot_.next(2 * index)},
            HilbertState{init_rot_.next(2 * index + 1)}};
  }

private:

  HilbertRotation<Dim> init_rot_;
  HilbertRotation<Dim> curr_rot_;

}; // class HilbertState

} // namespace impl

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
    using State = impl::HilbertState<Dim>;

    // Initialize sorting.
    const auto box = compute_bbox(points);
    iota_perm(points, perm);

    // Recursively partition the points along the Hilbert curve.
    par::TaskGroup tasks{};
    const auto impl = [&points, &tasks](this const auto& self,
                                        const Box& my_box,
                                        std::ranges::view auto my_perm,
                                        const State& state) -> void {
      if (std::size(my_perm) <= 1) return;

      // Split permutation along the current axis.
      const auto axis = state.axis();
      const auto reverse = state.dir();
      const auto center_coord = my_box.center()[axis];
      const auto [left_box, right_box] =
          my_box.split(axis, center_coord, reverse);
      const auto [left_perm, right_perm] =
          coord_bisection(points, my_perm, center_coord, axis, reverse);

      // Recursively sort the parts along the next axis.
      const auto [left_state, right_state] = state.next();
      constexpr size_t min_par_size = 50;
      tasks.run(std::bind_front(self, left_box, left_perm, left_state),
                std::ranges::size(left_perm) >= min_par_size ?
                    par::RunMode::parallel :
                    par::RunMode::sequential);
      tasks.run(std::bind_front(self, right_box, right_perm, right_state),
                std::ranges::size(right_perm) >= min_par_size ?
                    par::RunMode::parallel :
                    par::RunMode::sequential);
    };
    impl(box, std::views::all(perm), /*state=*/{});
    tasks.wait();
  }

}; // class HilbertCurveSort

/// Hilbert space filling curve spatial sort.
inline constexpr HilbertCurveSort hilbert_curve_sort{};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::geom
