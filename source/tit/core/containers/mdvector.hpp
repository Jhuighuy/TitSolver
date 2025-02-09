/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm> // IWYU pragma: keep
#include <array>
#include <functional>
#include <iterator>
#include <ranges>
#include <span>
#include <type_traits>
#include <vector>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/range_utils.hpp"
#include "tit/core/tuple_utils.hpp"

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Compatible multidimensional shape.
template<size_t Rank, class... Extents>
concept mdshape = can_pack_array<Rank, size_t, Extents...> &&
                  (packed_array_size_v<Extents...> == Rank);

/// Compatible multidimensional index.
template<size_t Rank, class... Indices>
concept mdindex = can_pack_array<Rank, size_t, Indices...>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Basic multidimensional non-owning container.
template<class Val, size_t Rank>
  requires (Rank >= 1)
class Mdspan final {
public:

  /// Shape type.
  using Shape = std::span<const size_t, Rank>;

  /// Construct a multidimensional span from values iterator and shape.
  template<std::contiguous_iterator ValIter>
  constexpr Mdspan(ValIter iter, Shape shape) noexcept
      // NOLINTNEXTLINE(misc-include-cleaner)
      : vals_{iter, std::ranges::fold_left(shape, 1, std::multiplies{})},
        shape_{shape} {}

  /// Amount of elements.
  constexpr auto size() const noexcept -> size_t {
    return vals_.size();
  }

  /// Span shape.
  constexpr auto shape() const noexcept -> Shape {
    return shape_;
  }

  /// Span data.
  /// @{
  constexpr auto data() noexcept -> Val* {
    return vals_.data();
  }
  constexpr auto data() const noexcept -> const Val* {
    return vals_.data();
  }
  /// @}

  /// Iterator pointing to the first span element.
  constexpr auto begin() const noexcept {
    return vals_.begin();
  }
  /// Iterator pointing to the element after the last span element.
  constexpr auto end() const noexcept {
    return vals_.end();
  }

  /// Reference to span element or sub-span.
  template<class... Indices>
    requires mdindex<Rank, const Indices&...>
  constexpr auto operator[](const Indices&... indices) const noexcept
      -> decltype(auto) {
    size_t offset = 0;
    for (const auto index_pack = make_array<Rank, size_t>(indices...);
         const auto [extent, index] : std::views::zip(shape_, index_pack)) {
      TIT_ASSERT(index < extent, "Index is out of range!");
      offset = index + offset * extent;
    }
    TIT_ASSERT(offset < vals_.size(), "Offset is out of range!");
    if constexpr (constexpr auto ResultRank = packed_array_size_v<Indices...>;
                  ResultRank == Rank) {
      return vals_[offset];
    } else {
      return tit::Mdspan{vals_.begin() + offset,
                         shape_.template subspan<ResultRank>()};
    }
  }

private:

  std::span<Val> vals_;
  Shape shape_;

}; // class Mdspan

template<std::contiguous_iterator ValIter, contiguous_fixed_size_range Shape>
Mdspan(ValIter, Shape&&)
    -> Mdspan<std::remove_reference_t<std::iter_reference_t<ValIter>>,
              range_fixed_size_v<Shape>>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Basic multidimensional owning container.
template<class Val, size_t Rank>
  requires (Rank >= 1)
class Mdvector final {
public:

  /// Shape type.
  using Shape = std::array<size_t, Rank>;

  /// Construct an empty multidimensional vector.
  constexpr Mdvector() noexcept = default;

  /// Construct a multidimensional vector with given shape and clear values.
  template<class... Extents>
    requires mdshape<Rank, Extents...>
  constexpr explicit Mdvector(const Extents&... extents) {
    assign(extents...);
  }

  /// Construct a multidimensional vector with given shape and values.
  template<std::forward_iterator ValIter, class... Extents>
    requires mdshape<Rank, Extents...>
  constexpr explicit Mdvector(ValIter iter, const Extents&... extents) {
    assign(iter, extents...);
  }

  /// Vector size.
  constexpr auto size() const noexcept -> size_t {
    return vals_.size();
  }

  /// Vector shape.
  constexpr auto shape() const noexcept -> const Shape& {
    return shape_;
  }

  /// Vector data.
  constexpr auto data(this auto& self) noexcept {
    return self.vals_.data();
  }

  /// Iterator pointing to the first vector element.
  constexpr auto begin(this auto& self) noexcept {
    return self.vals_.begin();
  }

  /// Iterator pointing to the element after the last vector element.
  constexpr auto end(this auto& self) noexcept {
    return self.vals_.end();
  }

  /// Clear the vector.
  constexpr void clear() noexcept {
    shape_.fill(0);
    vals_.clear();
  }

  /// Reshape the vector and clear values.
  template<class... Extents>
    requires mdshape<Rank, Extents...>
  constexpr void assign(const Extents&... extents) {
    shape_ = make_array<Rank, size_t>(extents...);
    vals_.clear();
    vals_.resize(std::ranges::fold_left(shape_, 1, std::multiplies{}));
  }

  /// Reshape the vector and assign values.
  template<std::forward_iterator ValIter, class... Extents>
    requires mdshape<Rank, Extents...>
  constexpr void assign(ValIter iter, const Extents&... extents) {
    assign(extents...);
    std::ranges::copy(iter, iter + size(), vals_.begin());
  }

  /// Reference to vector element or sub-vector span.
  template<class... Indices>
    requires mdindex<Rank, Indices...>
  constexpr auto operator[](this auto& self, Indices... indices) noexcept
      -> decltype(auto) {
    return Mdspan{self.vals_.begin(), self.shape_}[indices...];
  }

private:

  std::array<size_t, Rank> shape_{};
  std::vector<Val> vals_;

}; // class Mdvector

template<std::contiguous_iterator ValIter, class... Extents>
Mdvector(ValIter, const Extents&...)
    -> Mdvector<std::iter_value_t<ValIter>, packed_array_size_v<Extents...>>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit

// Enable borrowed range support for Mdspan, allowing it to be used safely
// in range expressions where the range must outlive the operation.
template<class Val, tit::size_t Rank>
constexpr bool std::ranges::enable_borrowed_range<tit::Mdspan<Val, Rank>> =
    true;
