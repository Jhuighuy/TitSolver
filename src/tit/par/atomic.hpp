/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License
 * See /LICENSE.md for license information.
 * SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include "tit/core/math.hpp"

namespace tit::par {

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/** Atomically perform addition and return what was stored before. */
template<class Val>
constexpr Val sync_fetch_and_add(Val& val, sub_result_t<Val> delta) noexcept {
  if consteval {
    const auto tmp = val;
    val += delta;
    return tmp;
  } else {
    // TODO: this is gcc/clang extension, not portable.
    return __sync_fetch_and_add(&val, delta);
  }
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

} // namespace tit::par
