/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/// @todo Tests are missing.
#pragma once

#include <iterator>

#include "tit/core/basic_types.hpp"
#include "tit/core/range_utils.hpp"

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Dynamic non-contiguous array. Items are stored in chunks.
template<class Item, size_t ChunkSize>
class ChunkedArray final {
public:

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Construct an empty array.
  constexpr ChunkedArray() noexcept = default;

  /// Construct an array with given size and initial value.
  constexpr explicit ChunkedArray(size_t size, const Item& init = Item{}) {
    assign(size, init);
  }

  /// Construct an array from an iterator and a sentinel.
  template<iterator_compatible_with<Item> Iter,
           std::sized_sentinel_for<Iter> Sent>
  constexpr explicit ChunkedArray(Iter first, Sent last) {
    assign(first, last);
  }

  /// Construct an array from an initializer list.
  constexpr ChunkedArray(std::initializer_list<Item> init) {
    assign(init);
  }

  /// Construct an array from a range.
  template<range_compatible_with<Item> Range>
    requires std::ranges::sized_range<Range>
  constexpr explicit ChunkedArray(std::from_range_t /*tag*/, Range&& range) {
    assign_range(std::forward<Range>(range));
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

private:

  std::vector<Item*> chunks_{};

}; // class ChunkedArray

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
