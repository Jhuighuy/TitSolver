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

#include "tit/utils/math.hpp"
#include "tit/utils/types.hpp"
#include "tit/utils/vec.hpp"

namespace tit {

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/******************************************************************************\
 ** Bounding box.
\******************************************************************************/
template<class Num, size_t Dim>
class BBox final {
public:

  Vec<Num, Dim> low, high;

  constexpr BBox() = default;
  constexpr BBox(Vec<Num, Dim> point) noexcept : low(point), high(point) {}
  constexpr BBox(Vec<Num, Dim> point1, Vec<Num, Dim> point2) noexcept
      : low(point1), high(point1) {
    update(point2);
  }

  constexpr void update(Vec<Num, Dim> point) noexcept {
    low = minimum(low, point);
    high = maximum(high, point);
  }
  constexpr void update(BBox<Num, Dim> bbox) noexcept {
    low = minimum(low, bbox.low);
    high = maximum(high, bbox.high);
  }

  constexpr auto clip(Vec<Num, Dim> point) const noexcept {
    point = maximum(low, point);
    point = minimum(high, point);
    return point;
  }

  constexpr auto center() const noexcept {
    return avg(low, high);
  }
  constexpr auto span() const noexcept {
    return high - low;
  }

}; // class BBox

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

} // namespace tit
