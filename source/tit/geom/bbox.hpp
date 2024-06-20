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

namespace tit::geom {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Bounding box.
template<class Vec>
class BBox final {
public:

  /// Construct a bounding box with both low and high points set to zero.
  constexpr BBox() = default;

  /// Construct a bounding box with both low and high points set to @p point.
  constexpr explicit BBox(const Vec& point) : low_{point}, high_{point} {}

  /// Construct a bounding box from two points @p p1, @p p2.
  constexpr BBox(const Vec& p1, const Vec& p2)
      : low_{minimum(p1, p2)}, high_{maximum(p1, p2)} {}

  /// Low bounding box point.
  constexpr auto low() const noexcept -> const Vec& {
    return low_;
  }

  /// High bounding box point.
  constexpr auto high() const noexcept -> const Vec& {
    return high_;
  }

  /// Bounding box center point.
  constexpr auto center() const -> Vec {
    return avg(low_, high_);
  }

  /// Bounding box extents.
  constexpr auto extents() const -> Vec {
    return high_ - low_;
  }

  /// Find the point inside of bounding box that is closest to @p point.
  constexpr auto clamp(Vec point) const -> Vec {
    point = maximum(low_, point);
    point = minimum(high_, point);
    return point;
  }

  /// Expand to align the edges with the given @p point.
  constexpr auto expand(const Vec& point) -> BBox& {
    low_ = minimum(low_, point);
    high_ = maximum(high_, point);
    return *this;
  }

  /// Extend on all sides by the given @p amount.
  constexpr auto grow(const vec_num_t<Vec>& amount) -> BBox& {
    TIT_ASSERT(amount >= 0, "Grow amount must be positive");
    low_ -= Vec(amount);
    high_ += Vec(amount);
    return *this;
  }

  /// Split the bbox into two.
  constexpr auto split(size_t axis, const vec_num_t<Vec>& val) const
      -> std::pair<BBox, BBox> {
    TIT_ASSERT(axis < vec_dim_v<Vec>, "Split axis is out of range!");
    TIT_ASSERT(low_[axis] <= val && val <= high_[axis],
               "Split value if out out range!");
    auto left = *this;
    left.high_[axis] = val;
    auto right = *this;
    right.low_[axis] = val;
    return std::pair{std::move(left), std::move(right)};
  }

private:

  Vec low_;
  Vec high_;

}; // class BBox

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::geom
