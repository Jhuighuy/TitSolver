/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <concepts>

#include <hwy/contrib/math/math-inl.h>
#include <hwy/highway.h>

#include "tit/core/math.hpp"

namespace tit {

namespace hn = hwy::HWY_NAMESPACE;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Fast math functions.
//

/// Fast `atan2` function overload.
template<std::floating_point Num>
[[gnu::always_inline]]
constexpr auto fast_atan2(Num y, Num x) noexcept -> Num {
  if consteval {
    return atan2(y, x);
  }
  constexpr hn::CappedTag<Num, 1> tag;
  return hn::GetLane(hn::Atan2(tag, hn::Set(tag, y), hn::Set(tag, x)));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Fast `log1p` function overload.
template<std::floating_point Num>
[[gnu::always_inline]]
constexpr auto fast_log1p(Num a) noexcept -> Num {
  if consteval {
    return log1p(a);
  }
  constexpr hn::CappedTag<Num, 1> tag;
  return hn::GetLane(hn::Log1p(tag, hn::Set(tag, a)));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
