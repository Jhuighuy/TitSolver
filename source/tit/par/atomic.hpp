/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include "tit/core/math.hpp"

namespace tit::par {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Atomically perform addition and return what was stored before.
template<class Val>
constexpr auto sync_fetch_and_add(Val& val,
                                  sub_result_t<Val, Val> delta) noexcept
    -> Val {
  if consteval {
    auto const tmp = val;
    val += delta;
    return tmp;
  } else { // NOLINT(*-else-after-return)
    // TODO: this is gcc/clang extension, not portable.
    return __sync_fetch_and_add(&val, delta); // NOLINT(*-vararg)
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::par
