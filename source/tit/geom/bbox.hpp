/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <concepts>
#include <tuple>
#include <utility>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/math.hpp"
#include "tit/core/vec.hpp"

namespace tit::geom {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Bounding box.
template<class Num, size_t Dim>
class BBox final {
private:

  Vec<Num, Dim> low_, high_;

public:

  // TODO: remove this constructor.
  constexpr BBox() = default;

  /// Construct bounding box with set of points.
  /// @{
  constexpr explicit BBox(Vec<Num, Dim> point) noexcept
      : low_{point}, high_{point} {}
  template<class... RestPoints>
    requires (std::constructible_from<Vec<Num, Dim>, RestPoints> && ...)
  constexpr explicit BBox(Vec<Num, Dim> point,
                          RestPoints... rest_points) noexcept
      : BBox{point} {
    (update(rest_points), ...);
  }
  /// @}

  /// Low bounding box point.
  constexpr auto low() const noexcept -> Vec<Num, Dim> {
    return low_;
  }

  /// High bounding box point.
  constexpr auto high() const noexcept -> Vec<Num, Dim> {
    return high_;
  }

  /// Bounding box center.
  constexpr auto center() const noexcept {
    return avg(low_, high_);
  }

  /// Bounding box extents.
  constexpr auto extents() const noexcept {
    return high_ - low_;
  }

  /// Update bounding box with point.
  constexpr auto update(Vec<Num, Dim> point) noexcept -> BBox& {
    low_ = minimum(low_, point);
    high_ = maximum(high_, point);
    return *this;
  }

  /// Extend bounding box with delta.
  constexpr auto extend(Vec<Num, Dim> delta) noexcept -> BBox& {
    *this = BBox{low_ - delta, high_ + delta};
    return *this;
  }

  /// Clamp point into the bounding box.
  constexpr auto clamp(Vec<Num, Dim> point) const noexcept {
    point = maximum(low_, point);
    point = minimum(high_, point);
    return point;
  }

  /// Find nearest point on the bounding box boundary.
  constexpr auto proj(Vec<Num, Dim> point) const noexcept {
    // TODO there should be much better implementation for this.
    point = clamp(point);
    auto const delta = point - center();
    size_t i = max_value_index(pow2(delta));
    point[i] = delta[i] >= 0 ? high_[i] : low_[i];
    return point;
  }

  /// Split bbox in two.
  /// @{
  constexpr auto split(size_t axis, Num val) const noexcept {
    TIT_ASSERT(axis < Dim, "Axis is out of range.");
    auto left = *this, right = *this;
    left.high_[axis] = val, right.low_[axis] = val;
    return std::tuple{left, right};
  }
  constexpr auto split(size_t axis, Vec<Num, Dim> point) const noexcept {
    TIT_ASSERT(axis < Dim, "Axis is out of range.");
    return split(axis, point[axis]);
  }
  /// @}

}; // class BBox

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Bounding box type for the specified vector type.
template<class Vec>
using bbox_t = decltype(BBox(std::declval<Vec>()));

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::geom
