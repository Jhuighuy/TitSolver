/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <utility>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/math.hpp"
#include "tit/core/vec.hpp"
#include "tit/core/vec/vec.hpp"

namespace tit::geom {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Bounding box.
template<class Num, size_t Dim>
class BBox final {
public:

  /// Bounding box point type.
  using Point = Vec<Num, Dim>;

  /// Construct a bounding box with both low and high points set to zero.
  constexpr BBox() = default;

  /// Construct a bounding box with both low and high points set to @p point.
  constexpr explicit BBox(const Point& point) : low_{point}, high_{point} {}

  /// Construct a bounding box from two points @p point_1, @p point_2.
  constexpr BBox(const Point& point_1, const Point& point_2)
      : low_{minimum(point_1, point_2)}, high_{maximum(point_1, point_2)} {}

  /// Low bounding box point.
  constexpr auto low() const noexcept -> const Point& {
    return low_;
  }

  /// High bounding box point.
  constexpr auto high() const noexcept -> const Point& {
    return high_;
  }

  /// Bounding box center point.
  constexpr auto center() const -> Point {
    return avg(low_, high_);
  }

  /// Bounding box extents.
  constexpr auto extents() const -> Point {
    return high_ - low_;
  }

  /// Find the point inside of bounding box that is closest to @p point.
  constexpr auto clamp(Point point) const -> Point {
    point = maximum(low_, point);
    point = minimum(high_, point);
    return point;
  }

  /// Expand to align the edges with the given @p point.
  constexpr auto expand(const Point& point) -> BBox& {
    low_ = minimum(low_, point);
    high_ = maximum(high_, point);
    return *this;
  }

  /// Extend on all sides by the given @p amount.
  constexpr auto grow(const Num& amount) -> BBox& {
    TIT_ASSERT(amount >= Num{0}, "Grow amount must be positive");
    low_ -= Point(amount);
    high_ += Point(amount);
    return *this;
  }

  /// Split the bbox into two.
  constexpr auto split(size_t axis,
                       const Num& val) const -> std::pair<BBox, BBox> {
    TIT_ASSERT(axis < Dim, "Split axis is out of range!");
    TIT_ASSERT(low_[axis] <= val && val <= high_[axis],
               "Split value if out out range!");
    auto left = *this;
    left.high_[axis] = val;
    auto right = *this;
    right.low_[axis] = val;
    return std::pair{std::move(left), std::move(right)};
  }

private:

  Point low_;
  Point high_;

}; // class BBox

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Bounding box type for the specified point type.
template<class Point>
using bbox_t = decltype(BBox(std::declval<Point>()));

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::geom
