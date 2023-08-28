/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*\
 * Copyright (C) 2020-2023 Oleg Butakov                                       *
 *                                                                            *
 * Permission is hereby granted, free of charge, to any person obtaining a    *
 * copy of this software and associated documentation files (the "Software"), *
 * to deal in the Software without restriction, including without limitation  *
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,   *
 * and/or sell copies of the Software, and to permit persons to whom the      *
 * Software is furnished to do so, subject to the following conditions:       *
 *                                                                            *
 * The above copyright notice and this permission notice shall be included in *
 * all copies or substantial portions of the Software.                        *
 *                                                                            *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR *
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,   *
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL    *
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER *
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING    *
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER        *
 * DEALINGS IN THE SOFTWARE.                                                  *
\*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

#pragma once

#include <concepts>
#include <tuple>

#include "tit/core/math.hpp"
#include "tit/core/types.hpp"
#include "tit/core/vec.hpp"

namespace tit {

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/******************************************************************************\
 ** Bounding box.
\******************************************************************************/
template<class Num, size_t Dim>
class BBox final {
public:

  /** Low bounding box point. */
  Vec<Num, Dim> low;
  /** High bounding box point. */
  Vec<Num, Dim> high;

  // TODO: remove this constructor.
  constexpr BBox() = default;

  /** Construct bounding box with set of points. */
  /** @{ */
  constexpr BBox(Vec<Num, Dim> point) noexcept : low{point}, high{point} {}
  template<class... RestPoints>
    requires (std::constructible_from<Vec<Num, Dim>, RestPoints> && ...)
  constexpr BBox(Vec<Num, Dim> point, RestPoints... rest_points) noexcept
      : BBox{point} {
    (update(rest_points), ...);
  }
  /** @} */

  /** Bounding box center. */
  constexpr auto center() const noexcept {
    return avg(low, high);
  }
  /** Bounding box extents. */
  constexpr auto extents() const noexcept {
    return high - low;
  }

  /** Update bounding box with point. */
  constexpr auto update(Vec<Num, Dim> point) noexcept -> BBox& {
    low = minimum(low, point);
    high = maximum(high, point);
    return *this;
  }

  /** Clamp point into the bounding box. */
  constexpr auto clamp(Vec<Num, Dim> point) const noexcept {
    point = maximum(low, point);
    point = minimum(high, point);
    return point;
  }
  /** Find nearest point on the bounding box boundary. */
  constexpr auto proj(Vec<Num, Dim> point) const noexcept {
    // TODO there should be much better implementation for this.
    point = clamp(point);
    const auto delta = point - center();
    size_t i = argmax_value(pow2(delta));
    point[i] = delta[i] >= 0 ? high[i] : low[i];
    return point;
  }

  /** Split bbox in two. */
  constexpr auto split(Vec<Num, Dim> point, size_t axis) const noexcept {
    auto left = *this, right = *this;
    left.high[axis] = point[axis], right.low[axis] = point[axis];
    return std::tuple{left, right};
  }

}; // class BBox

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

} // namespace tit
