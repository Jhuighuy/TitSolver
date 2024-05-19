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
#include "tit/core/par.hpp"
#include "tit/core/profiler.hpp"
#include "tit/core/vec.hpp"

#include "tit/geom/bbox.hpp"

namespace tit::geom {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Morton space filling curve spatial ordering.
template<std::ranges::view Points>
  requires std::ranges::sized_range<Points> &&
           std::ranges::random_access_range<Points> &&
           is_vec_v<std::ranges::range_value_t<Points>>
class MortonCurveOrdering final {
public:

  /// Point type.
  using Point = std::ranges::range_value_t<Points>;

  /// Bounding box type.
  using PointBBox = bbox_t<Point>;

  /// Numeric type used by the point type.
  static constexpr auto Dim = vec_dim_v<Point>;

  /// Initialize and build Morton SFC curve ordering.
  explicit MortonCurveOrdering(Points points) : points_{std::move(points)} {
    TIT_PROFILE_SECTION("MortonCurveOrdering::ctor()");
    if (std::ranges::empty(points_)) return;
    // Initialize identity point ordering.
    indices_.resize(std::ranges::size(points_));
    std::ranges::copy(std::views::iota(size_t{0}, indices_.size()),
                      indices_.begin());
    // Compute bounding box.
    auto bbox = BBox{points_[0]};
    for (const auto& p : points_ | std::views::drop(1)) bbox.expand(p);
    // Compute the root bounding box and build ordering.
    par::TaskGroup tasks{};
    build_indices_<0>(tasks, bbox, indices_);
    tasks.wait();
  }

  void GetHilbertElementOrdering(std::vector<size_t>& ordering) {
    ordering = std::move(indices_);
  }

private:

  // Compute the bounding box.
  auto build_bbox_() noexcept {
    auto bbox = BBox{points_[0]};
    for (const auto& p : points_ | std::views::drop(1)) bbox.update(p);
    return bbox;
  }

  // Build permutation using a divide and conquire approach.
  template<size_t Axis>
    requires (Axis < Dim)
  void build_indices_(par::TaskGroup& tasks, //
                      PointBBox bbox,
                      std::span<size_t> range) noexcept {
    if (range.size() <= 1) return;
    const auto center = bbox.center();
    // Split ordering along the given axis.
    const auto [left_bbox, right_bbox] = bbox.split(Axis, center[Axis]);
    const auto is_left = [value = center[Axis], this](size_t index) {
      return points_[index][Axis] <= value;
    };
    const std::span right_range(std::ranges::partition(range, is_left));
    const std::span left_range(range.begin(), right_range.begin());
    // Recursively split the head and tail along the next axis.
    constexpr auto NextAxis = (Axis + 1) % Dim;
    tasks.run(is_async_(left_range), [=, &tasks, this] {
      build_indices_<NextAxis>(tasks, left_bbox, left_range);
    });
    tasks.run(is_async_(right_range), [=, &tasks, this] {
      build_indices_<NextAxis>(tasks, right_bbox, right_range);
    });
  }

  // Should the ordering be done in parallel?
  static constexpr auto is_async_(std::span<size_t> range) noexcept -> bool {
    static constexpr size_t TooSmall = 50;
    return range.size() >= TooSmall;
  }

  Points points_;
  std::vector<size_t> indices_;

}; // class MortonCurveOrdering

// Wrap a viewable range into a view on construction.
template<class Points, class... Args>
MortonCurveOrdering(Points&&,
                    Args...) -> MortonCurveOrdering<std::views::all_t<Points>>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::geom
