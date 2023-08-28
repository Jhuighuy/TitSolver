/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
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
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm> // IWYU pragma: keep
#include <cstdlib>   // IWYU pragma: keep
#include <memory>    // IWYU pragma: keep
#include <mutex>     // IWYU pragma: keep
#include <new>       // IWYU pragma: keep

#include "tit/core/config.hpp" // IWYU pragma: keep
#include "tit/core/types.hpp"  // IWYU pragma: keep

#if !TIT_ENABLE_TBB
#error Pool allocator requires TBB for now.
#endif
#define TBB_PREVIEW_MEMORY_POOL 1
#include <oneapi/tbb/memory_pool.h>

namespace tit {

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/******************************************************************************\
 ** Simple pool (arena) allocator.
\******************************************************************************/
#if TIT_ENABLE_TBB
template<class Val>
class PoolAllocator :
    public tbb::memory_pool_allocator<Val,
                                      tbb::memory_pool<std::allocator<Val>>> {
private:

  tbb::memory_pool<std::allocator<Val>> _mem_pool;

public:

  PoolAllocator()
      : tbb::memory_pool_allocator<Val, tbb::memory_pool<std::allocator<Val>>>{
            _mem_pool} {}

}; // class PoolAllocator
#else
template<class Val>
class PoolAllocator final {
private:

  // Individual allocations would be aligned to this value.
  static constexpr auto _word_size = round_up(size_t{16}, sizeof(Val));
  // Amount of the block.
  static constexpr auto _block_size = round_up(size_t{64 * 1024}, _word_size);

  // Number of bytes left in current block of storage.
  size_t _remaining = 0;
  // Pointer to base of current block of storage.
  void* _base = nullptr;
  // Current location in block to next allocate.
  void* _loc = nullptr;
  // Mutex to make this guy thread-safe.
  std::mutex _mutex;

public:

  /** Construct pool allocator. */
  PoolAllocator() = default;

  /** Move-construct the pool allocator. */
  PoolAllocator(PoolAllocator&&) = default;
  /** Move-assign the K-dimensional tree. */
  auto operator=(PoolAllocator&&) -> PoolAllocator& = default;

  /** Pool allocator is non-copy-constructible. */
  PoolAllocator(const PoolAllocator&) = delete;
  /** Pool allocator is non-copyable. */
  auto operator=(const PoolAllocator&) -> PoolAllocator& = delete;

  /** Destroy pool allocator and free all memory. */
  constexpr ~PoolAllocator() {
    while (_base != nullptr) {
      // Get pointer to previous block.
      const auto prev = *static_cast<void**>(_base);
      std::free(_base);
      _base = prev;
    }
  }

  constexpr Val* allocate(size_t count = 1) {
    auto size = sizeof(Val) * count;
    size = (size + (_word_size - 1)) & ~(_word_size - 1);
    const auto lock = std::unique_lock{_mutex};
    if (size > _remaining) {
      /* Allocate new storage. */
      const auto blocksize =
          std::max(size + sizeof(void*) + (_word_size - 1), _block_size);
      const auto m = std::malloc(blocksize);
      if (m == nullptr) throw std::bad_alloc();
      static_cast<void**>(m)[0] = _base;
      _base = m;
      _remaining = blocksize - sizeof(void*);
      _loc = (static_cast<char*>(m) + sizeof(void*));
    }
    auto rloc = _loc;
    _loc = static_cast<char*>(_loc) + size;
    _remaining -= size;
    return static_cast<Val*>(rloc);
  }

}; // class PoolAllocator
#endif

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

} // namespace tit
