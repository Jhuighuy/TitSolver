/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// IWYU pragma: private, include "tit/core/par.hpp"
#pragma once

#include <concepts>
#include <type_traits>

#include "tit/core/type_traits.hpp"

namespace tit::par {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Atomically perform addition and return what was stored before.
template<class Val>
  requires std::integral<Val> || std::is_pointer_v<Val>
auto fetch_and_add(Val& val, sub_result_t<Val> delta) noexcept -> Val {
  // NOLINTNEXTLINE(*-pro-type-vararg)
  return __atomic_fetch_add( //
      &val,
      delta,
      /*memorder=*/__ATOMIC_RELAXED);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Atomically compare and exchange the value.
template<class Val>
  requires std::integral<Val> || std::is_pointer_v<Val>
auto compare_exchange(Val& val,
                      std::type_identity_t<Val> expected,
                      std::type_identity_t<Val> desired) noexcept -> bool {
  // NOLINTNEXTLINE(*-pro-type-vararg)
  return __atomic_compare_exchange_n( //
      &val,
      &expected,
      desired,
      /*weak=*/false,
      /*success_memorder=*/__ATOMIC_RELAXED,
      /*failure_memorder=*/__ATOMIC_RELAXED);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::par
