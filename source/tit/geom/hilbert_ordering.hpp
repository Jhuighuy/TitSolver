/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm>
#include <ranges>
#include <utility>
#include <vector>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/profiler.hpp"
#include "tit/core/vec.hpp"

#include "tit/geom/bbox.hpp"

#include "tit/par/task_group.hpp"

namespace tit::geom {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Z-curve spatial ordering.
template<std::ranges::view Points>
  requires std::ranges::sized_range<Points> &&
           std::ranges::random_access_range<Points> &&
           is_vec_v<std::ranges::range_value_t<Points>>
class ZCurveOrdering {
public:

  /// Point type.
  using Point = std::ranges::range_value_t<Points>;

  /// Bounding box type.
  using PointBBox = bbox_t<Point>;

  /// Numeric type used by the point type.
  static constexpr auto Dim = vec_dim_v<Point>;

private:

  Points points_;
  std::vector<size_t> point_perm_;

public:

  /// Initialize and build the K-dimensional tree.
  ///
  /// @param max_leaf_size Maximum amount of points in the leaf node.
  constexpr explicit ZCurveOrdering(Points points)
      : points_{std::move(points)} {
    TIT_PROFILE_SECTION("tit::ZCurveOrdering::ZCurveOrdering()");
    if (std::ranges::empty(points_)) return;
    // Initialize identity points permutation.
    auto const size = std::ranges::size(points_);
    point_perm_.resize(size);
    std::ranges::copy(std::views::iota(0UZ, size), point_perm_.begin());
    // Compute bounding box.
    auto bbox = BBox{points_[0]};
    for (auto const& p : points_ | std::views::drop(1)) bbox.update(p);
    // Compute the root bounding box and build ordering.
    // TODO: refactor with `std::span`.
    // NOLINTBEGIN(*-bounds-pointer-arithmetic)
    partition_(point_perm_.data(), //
               point_perm_.data() + point_perm_.size(), bbox);
    // NOLINTEND(*-bounds-pointer-arithmetic)
  }

  void GetHilbertElementOrdering(std::vector<size_t>& ordering) {
    ordering = std::move(point_perm_);
  }

private:

  // Build the K-dimensional subtree.
  // On the input `bbox` contains a rough estimate that was guessed by the
  // caller. On return it contains an exact bounding box of the subtree.
  constexpr void partition_(size_t* first, size_t* last,
                            PointBBox bbox) noexcept {
    TIT_ASSERT(first <= last, "Invalid point iterators.");
    if (last - first <= 1) return;
    auto const center = bbox.center();
    if constexpr (Dim == 2) {
      auto const in_upper_part = [&](size_t index) {
        return points_[index][1] > center[1];
      };
      auto const to_the_left = [&](size_t index) {
        return points_[index][0] < center[0];
      };
      // Split subtree vertically.
      auto const [lower_bbox, upper_bbox] = bbox.split(1, center);
      auto* const upper = first;
      auto* const lower = std::partition(first, last, in_upper_part);
      // Split upper part horizontally.
      auto const [upper_left_bbox, //
                  upper_right_bbox] = upper_bbox.split(0, center);
      auto* const upper_left = upper;
      auto* const upper_right = std::partition(upper, lower, to_the_left);
      // Split lower part horizontally.
      auto const [lower_left_bbox, //
                  lower_right_bbox] = lower_bbox.split(0, center);
      auto* const lower_left = lower;
      auto* const lower_right = std::partition(lower, last, to_the_left);
      // Recursively build quadrants.
      par::invoke(
          [=, this] { partition_(upper_left, upper_right, upper_left_bbox); },
          [=, this] { partition_(upper_right, lower_left, upper_right_bbox); },
          [=, this] { partition_(lower_left, lower_right, lower_left_bbox); },
          [=, this] { partition_(lower_right, last, lower_right_bbox); });
    }
  }

}; // class ZCurveOrdering

// Wrap a viewable range into a view on construction.
template<class Points, class... Args>
ZCurveOrdering(Points&&, Args...) -> ZCurveOrdering<std::views::all_t<Points>>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::geom
