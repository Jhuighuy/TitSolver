/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/// @todo Tests are missing.
#pragma once

#include <algorithm>
#include <array>
#include <functional>
#include <ranges>
#include <span>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/containers/utils.hpp"
#include "tit/core/tuple_utils.hpp"

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Compatible multidimensional shape.
template<size_t Rank, class... Extents>
concept md_shape = can_pack_array<Rank, size_t, Extents...> &&
                   (packed_array_size_v<Extents...> == Rank);

/// Compatible multidimensional index.
template<size_t Rank, class... Indices>
concept md_index = can_pack_array<Rank, size_t, Indices...>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Non-owning multidimensional container view.
template<class Item, size_t Rank>
  requires std::is_object_v<Item> && (Rank > 0)
class MDSpan final {
public:

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Span shape.
  constexpr auto shape() const noexcept -> const std::array<size_t, Rank>& {
    return shape_;
  }

  /// Span size (total number of items).
  constexpr auto size() const noexcept -> size_t {
    return std::ranges::fold_left(shape_, 1, std::multiplies{});
  }

  /// Is span empty?
  constexpr auto empty() const noexcept -> bool {
    return size() == 0;
  }

  /// Span data.
  constexpr auto data() const noexcept -> Item* //
      requires (Rank == 1) { return data_; }

  /// Access subspan (or item) at indices.
  template<class... Indices>
    requires md_index<Rank, const Indices&...>
  constexpr auto operator[](const Indices&... indices) const noexcept
      -> decltype(auto) {
    size_t offset = 0;
    for (const auto index_pack = make_array<Rank, size_t>(indices...);
         const auto [extent, index] : std::views::zip(shape_, index_pack)) {
      TIT_ASSERT(index < extent, "Index is out of range!");
      offset = index + offset * extent;
    }
    TIT_ASSERT(offset < size(), "Offset is out of range!");
    // NOLINTBEGIN(*-pointer-arithmetic)
    if constexpr (constexpr auto Delta = packed_array_size_v<Indices...>;
                  Delta == Rank) {
      return data_[offset];
    } else {
      return MDSpan<Item, Rank - Delta>{
          data_ + offset,
          shape_.begin() + Delta,
          shape_.end(),
      };
    }
    // NOLINTEND(*-pointer-arithmetic)
  }

  /// Iterator pointing to the subspan (or item).
  constexpr auto begin(this auto& self) noexcept {
    return RandomAccessIterator{self};
  }

  /// Iterator pointing after the last element.
  constexpr auto end(this auto& self) noexcept {
    return RandomAccessIterator{self, self.size()};
  }

  /// All the span items.
  constexpr auto all() const noexcept -> std::span<Item> {
    return std::span{data_, size()};
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

private:

  Item* data_ = nullptr;
  [[no_unique_address]] std::array<size_t, Rank> shape_{};

}; // class MDSpan

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
