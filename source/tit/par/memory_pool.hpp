/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <concepts>
#include <memory>
#include <type_traits>

#ifndef TBB_PREVIEW_MEMORY_POOL
#define TBB_PREVIEW_MEMORY_POOL 1
#endif

#include <oneapi/tbb/memory_pool.h>

#include "tit/core/checks.hpp"

namespace tit::par {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Thread-safe and scalable memory pool (arena).
template<class Val>
  requires std::is_object_v<Val> && std::is_trivially_destructible_v<Val>
class MemoryPool final {
public:

  /// Allocate and initialize the new value from @p args.
  ///
  /// @returns Pointer to the allocated memory or `nullptr`.
  template<class... Args>
    requires std::constructible_from<Val, Args&&...>
  [[nodiscard]]
  auto create(Args&&... args) -> Val* {
    TIT_ASSERT(pool_ != nullptr, "Memory pool was moved away!");
    auto* const ptr = static_cast<Val*>(pool_->malloc(sizeof(Val)));
    if (ptr != nullptr) std::construct_at(ptr, std::forward<Args>(args)...);
    return ptr;
  }

  /// Free memory that was previously allocated inside of the current pool.
  ///
  /// @note Values are not deinitialized: no destructors are called!
  void destroy(Val* pointer) {
    TIT_ASSERT(pool_ != nullptr, "Memory pool was moved away!");
    pool_->free(pointer);
  }

private:

  std::unique_ptr<tbb::memory_pool<std::allocator<Val>>> pool_ =
      std::make_unique<typename decltype(pool_)::element_type>();

}; // class MemoryPool

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::par
