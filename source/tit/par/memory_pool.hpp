/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License
 * See /LICENSE.md for license information.
 * SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <concepts>
#include <memory>
#include <type_traits> // IWYU pragma: keep

#define TBB_PREVIEW_MEMORY_POOL 1
#include <oneapi/tbb/memory_pool.h>

#include "tit/core/assert.hpp"
#include "tit/core/types.hpp"

namespace tit::par {

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/******************************************************************************\
 ** @brief Thread-safe and scalable memory pool (arena).
\******************************************************************************/
template<class Val>
  requires std::is_object_v<Val>
class MemoryPool final {
private:

  // Unfortunately, `tbb::memory_pool` is not "movable" nor "move_constructible"
  // due to it's implementation details. It is not even "relocatable": it's
  // implementation relies on it's own address. In order to make our class
  // movable, I'll play safe and wrap it into the unique pointer.
  using TbbMemoryPool_ = tbb::memory_pool<std::allocator<Val>>;
  std::unique_ptr<TbbMemoryPool_> pool_{};
  static_assert(
      !std::movable<TbbMemoryPool_>,
      "A reminder to update implementation once memory pool becomes movable.");

public:

  /** Construct the memory pool with specified allocator. */
  MemoryPool() : pool_{new TbbMemoryPool_{}} {}

  /** Move-construct the memory pool. */
  MemoryPool(MemoryPool&&) = default;
  /** Move-assign the pool allocator. */
  auto operator=(MemoryPool&&) -> MemoryPool& = default;

  /** Memory pool is not copy-constructible. */
  MemoryPool(const MemoryPool&) = delete;
  /** Memory pool is not copyable. */
  auto operator=(const MemoryPool&) -> MemoryPool& = delete;

  /** Destroy memory pool and free all memory. */
  ~MemoryPool() = default;

  /** Allocate the specified amount of values.
   ** @note Values are not initialized: no constructors are called!
   ** @returns Pointer to the allocated memory or `nullptr`. */
  [[nodiscard]] auto allocate(size_t count = 1) -> Val* {
    TIT_ASSERT(pool_ != nullptr, "Memory pool was moved away!");
    const auto num_bytes = count * sizeof(Val);
    return static_cast<Val*>(pool_->malloc(num_bytes));
  }

  /** Free memory that was previously allocated inside of the current pool.
   ** @note Values are not deinitialized: no destructors are called! */
  void deallocate(Val* pointer) {
    TIT_ASSERT(pool_ != nullptr, "Memory pool was moved away!");
    pool_->free(pointer);
  }

}; // class MemoryPool

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

} // namespace tit::par