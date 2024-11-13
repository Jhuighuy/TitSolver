/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <array>
#include <functional>
#include <tuple>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/math.hpp"
#include "tit/core/type_utils.hpp"
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

  /// Extend on all sides by the given @p amount.
  /// @{
  constexpr auto grow(const Vec& amount) -> BBox& {
    TIT_ASSERT(amount >= Vec(0), "Grow amount must be positive!");
    low_ -= amount;
    high_ += amount;
    return *this;
  }
  constexpr auto grow(const vec_num_t<Vec>& amount) -> BBox& {
    return grow(Vec(amount));
  }
  /// @}

  /// Shrink bounding box by the given @p amount.
  /// @{
  constexpr auto shrink(const Vec& amount) -> BBox& {
    TIT_ASSERT(amount >= Vec(0), "Shrink amount must be positive!");
    low_ += amount;
    high_ -= amount;
    return *this;
  }
  constexpr auto shrink(const vec_num_t<Vec>& amount) -> BBox& {
    return shrink(Vec(amount));
  }
  /// @}

  /// Expand to align the edges with the given @p point.
  constexpr auto expand(const Vec& point) -> BBox& {
    low_ = minimum(low_, point);
    high_ = maximum(high_, point);
    return *this;
  }

  /// Intersect the bounding box with another @p bbox.
  constexpr auto intersect(const BBox& bbox) -> BBox& {
    low_ = maximum(low_, bbox.low());
    high_ = minimum(high_, bbox.high());
    return *this;
  }

  /// Join the bounding box with another @p bbox.
  constexpr auto join(const BBox& bbox) -> BBox& {
    low_ = minimum(low_, bbox.low());
    high_ = maximum(high_, bbox.high());
    return *this;
  }

  /// Split the bounding box into two parts by the plane.
  constexpr auto split(size_t axis,
                       const vec_num_t<Vec>& val,
                       bool reverse = false) const -> std::array<BBox, 2> {
    TIT_ASSERT(axis < vec_dim_v<Vec>, "Split axis is out of range!");
    TIT_ASSERT(val >= low_[axis], "Split value is less than lower bound!");
    TIT_ASSERT(val <= high_[axis], "Split value is greater than upper bound!");
    auto left = *this;
    left.high_[axis] = val;
    auto right = *this;
    right.low_[axis] = val;
    if (reverse) return {right, left};
    return {left, right};
  }

  /// Split the bounding box into parts by the point.
  constexpr auto split(const Vec& point) const
      -> std::array<BBox, (1 << vec_dim_v<Vec>)> {
    TIT_ASSERT(point >= low_, "Split point is below lower bounds!");
    TIT_ASSERT(point <= high_, "Split point is above upper bounds!");
    return [&point]<size_t Axis>(this const auto& self,
                                 value_constant_t<Axis> /*axis*/,
                                 const auto&... boxes) {
      auto split_boxes = array_cat(boxes.split(Axis, point[Axis])...);
      if constexpr (Axis == vec_dim_v<Vec> - 1) {
        return split_boxes;
      } else {
        return std::apply(std::bind_front(self, value_constant_t<Axis + 1>{}),
                          split_boxes);
      }
    }(value_constant_t<size_t{0}>{}, *this);
  }

private:

  Vec low_;
  Vec high_;

}; // class BBox

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::geom
