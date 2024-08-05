/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// IWYU pragma: private, include "tit/core/simd.hpp"
#pragma once

#include <algorithm>
#include <cstdlib>
#include <new>
#include <type_traits>

#include "tit/core/basic_types.hpp"
#include "tit/core/simd/traits.hpp"

namespace tit::simd {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// SIMD-aligned allocator.
template<class Val>
class Allocator {
public:

  using value_type = Val;
  using size_type = size_t;
  using difference_type = ssize_t;
  using propagate_on_container_move_assignment = std::true_type;

  /// Allocate memory.
  constexpr auto allocate(size_t count,
                          const void* /*hint*/ = nullptr) -> Val* {
    const auto alignment = std::max(max_reg_byte_width_v, alignof(Val));
    auto* const ptr = // NOLINT(*-no-malloc,*-owning-memory)
        static_cast<Val*>(std::aligned_alloc(alignment, count * sizeof(Val)));
    if (ptr == nullptr) throw std::bad_alloc{};
    return ptr;
  }

  /// Deallocate memory.
  constexpr void deallocate(Val* ptr, size_type /*count*/) {
    std::free(ptr); // NOLINT(*-no-malloc,*-owning-memory)
  }

private:

  size_t alignment_;

}; // class Allocator

/// Allocators are always equal.
template<class Val1, class Val2>
constexpr auto operator==(const Allocator<Val1>& /*alloc_1*/,
                          const Allocator<Val2>& /*alloc_2*/) noexcept -> bool {
  return true;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::simd
