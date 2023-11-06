/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License
 * See /LICENSE.md for license information.
 * SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include "tit/core/config.hpp"
#include "tit/core/math.hpp"
#include "tit/core/types.hpp"

#if TIT_ENABLE_TBB
#define TBB_PREVIEW_MEMORY_POOL 1
#include <oneapi/tbb/memory_pool.h>
#else
#include <algorithm>
#include <mutex>
#endif

namespace tit {

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

#if TIT_ENABLE_TBB

/******************************************************************************\
 ** Simple pool (arena) allocator.
\******************************************************************************/
template<class Val>
class PoolAllocator {
private:

  tbb::memory_pool<std::allocator<Val>> pool_;
  tbb::memory_pool_allocator<Val, tbb::memory_pool<std::allocator<Val>>> alloc_;

public:

  /** Construct pool allocator. */
  constexpr PoolAllocator() : pool_{}, alloc_{pool_} {}

  /** Move-construct the pool allocator. */
  PoolAllocator(PoolAllocator&&) = default;
  /** Move-assign the pool allocator. */
  auto operator=(PoolAllocator&&) -> PoolAllocator& = default;

  /** Pool allocator is non-copy-constructible. */
  PoolAllocator(const PoolAllocator&) = delete;
  /** Pool allocator is non-copyable. */
  auto operator=(const PoolAllocator&) -> PoolAllocator& = delete;

  /** Destroy pool allocator and free all memory. */
  ~PoolAllocator() = default;

  /** Allocate space for the given amount of objects. */
  constexpr Val* allocate(size_t count = 1) {
    return alloc_.allocate(count);
  }

}; // class PoolAllocator

#else

/******************************************************************************\
 ** Simple pool (arena) allocator.
\******************************************************************************/
template<class Val>
class PoolAllocator final {
private:

  // Individual allocations would be aligned to this value.
  static constexpr auto word_size_ = align(size_t{16}, sizeof(Val));
  // Amount of the block.
  static constexpr auto block_size_ = align(size_t{64} * 1024, word_size_);

  size_t remaining_ = 0; // Number of bytes left in current block of storage.
  void* base_ = nullptr; // Pointer to base of current block of storage.
  void* loc_ = nullptr;  // Current location in block to next allocate.
  std::mutex mutex_;     // Mutex to make this guy thread-safe.

public:

  /** Construct pool allocator. */
  PoolAllocator() = default;

  /** Move-construct the pool allocator. */
  PoolAllocator(PoolAllocator&&) = default;
  /** Move-assign the pool allocator. */
  auto operator=(PoolAllocator&&) -> PoolAllocator& = default;

  /** Pool allocator is non-copy-constructible. */
  PoolAllocator(const PoolAllocator&) = delete;
  /** Pool allocator is non-copyable. */
  auto operator=(const PoolAllocator&) -> PoolAllocator& = delete;

  /** Destroy pool allocator and free all memory. */
  /*constexpr*/ ~PoolAllocator() {
    while (base_ != nullptr) {
      // Get pointer to previous block.
      auto* const prev = *static_cast<void**>(base_);
      delete[] static_cast<char*>(base_);
      base_ = prev;
    }
  }

  constexpr auto allocate(size_t count = 1) -> Val* {
    auto size = align(sizeof(Val) * count, word_size_);
    size = (size + (word_size_ - 1)) & ~(word_size_ - 1);
    const auto lock = std::unique_lock{mutex_};
    if (size > remaining_) {
      /* Allocate new storage. */
      const auto blocksize =
          std::max(size + sizeof(void*) + (word_size_ - 1), block_size_);
      auto* const m = static_cast<void*>(new char[blocksize]);
      static_cast<void**>(m)[0] = base_;
      base_ = m;
      remaining_ = blocksize - sizeof(void*);
      loc_ = (static_cast<char*>(m) + sizeof(void*));
    }
    auto* rloc = loc_;
    loc_ = static_cast<char*>(loc_) + size;
    remaining_ -= size;
    return static_cast<Val*>(rloc);
  }

}; // class PoolAllocator
#endif

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

} // namespace tit
