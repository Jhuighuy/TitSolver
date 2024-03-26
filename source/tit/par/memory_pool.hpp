/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <concepts> // IWYU pragma: keep
#include <memory>
#include <type_traits>

#define TBB_PREVIEW_MEMORY_POOL 1
#include <oneapi/tbb/memory_pool.h>

#include "tit/core/checks.hpp"

namespace tit::par {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Thread-safe and scalable memory pool (arena).
template<class Val>
  requires std::is_object_v<Val> && std::is_trivially_destructible_v<Val>
class MemoryPool final {
public:

  /// Construct the memory pool with specified allocator.
  MemoryPool() : pool_{new TbbMemoryPool()} {}

  /// Move-construct the memory pool.
  MemoryPool(MemoryPool&&) = default;

  /// Move-assign the pool allocator.
  auto operator=(MemoryPool&&) -> MemoryPool& = default;

  /// Memory pool is not copy-constructible.
  MemoryPool(MemoryPool const&) = delete;

  /// Memory pool is not copyable.
  auto operator=(MemoryPool const&) -> MemoryPool& = delete;

  /// Destroy memory pool and free all memory.
  ~MemoryPool() = default;

  /// Allocate and initialize the new value from @p args.
  /// @returns Pointer to the allocated memory or `nullptr`.
  template<class... Args>
    requires std::constructible_from<Val, Args&&...>
  [[nodiscard]] auto create(Args&&... args) -> Val* {
    TIT_ASSERT(pool_ != nullptr, "Memory pool was moved away!");
    auto* const ptr = static_cast<Val*>(pool_->malloc(sizeof(Val)));
    if (ptr != nullptr) std::construct_at(ptr, std::forward<Args>(args)...);
    return ptr;
  }

  /// Free memory that was previously allocated inside of the current pool.
  /// @note Values are not deinitialized: no destructors are called!
  void destroy(Val* pointer) {
    TIT_ASSERT(pool_ != nullptr, "Memory pool was moved away!");
    pool_->free(pointer);
  }

private:

  using TbbMemoryPool = tbb::memory_pool<std::allocator<Val>>;

  // Unfortunately, `tbb::memory_pool` is not "movable" nor
  // "move_constructible": it's implementation relies on it's own address.
  // In order to make our class `movable`, I'll play safe and wrap it into the
  // unique pointer.
  std::unique_ptr<TbbMemoryPool> pool_{};

}; // class MemoryPool

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::par
