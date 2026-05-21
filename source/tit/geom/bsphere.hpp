/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include "tit/core/assert.hpp"
#include "tit/core/vec.hpp"
#include "tit/geom/bbox.hpp"

namespace tit::geom {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Bounding sphere.
template<class Vec>
class BSphere final {
public:

  /// Numeric type of the bounding sphere.
  using Num = vec_num_t<Vec>;

  /// Construct a bounding sphere with center and radius set to zero.
  constexpr BSphere() = default;

  /// Construct a bounding sphere with the given center and radius set to zero.
  constexpr explicit BSphere(const Vec& center) : center_{center} {}

  /// Construct a bounding sphere with the given center and radius.
  constexpr BSphere(const Vec& center, const Num& radius)
      : center_{center}, radius_{radius} {
    TIT_ASSERT(radius >= Num{0}, "Radius must be non-negative!");
  }

  /// Center of the bounding sphere.
  constexpr auto center() const noexcept -> const Vec& {
    return center_;
  }

  /// Radius of the bounding sphere.
  constexpr auto radius() const noexcept -> const Num& {
    return radius_;
  }

  /// Get the bounding box of the bounding sphere.
  constexpr auto box() const noexcept -> BBox<Vec> {
    return BBox{center()}.grow(radius());
  }

  /// Check if the sphere contains the point (inclusive).
  constexpr auto contains(const Vec& point) const noexcept -> bool {
    return norm2(point - center()) <= pow2(radius());
  }

private:

  Vec center_;
  Num radius_{};

}; // class BSphere

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::geom
