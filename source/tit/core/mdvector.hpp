/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <array>
#include <concepts>
#include <iterator>
#include <ranges>
#include <span>
#include <type_traits>
#include <utility>
#include <vector>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/type_traits.hpp"
#include "tit/core/utils.hpp"

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Compatible multidimensional shape.
template<size_t Rank, class... Extents>
concept mdshape = (sizeof...(Extents) <= Rank) &&
                  (std::convertible_to<Extents, size_t> && ...);

/// Compatible multidimensional index.
template<size_t Rank, class... Indices>
concept mdindex = in_range_v<sizeof...(Indices), 1, Rank> &&
                  (std::convertible_to<Indices, size_t> && ...);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Basic multidimensional non-owning container.
template<class Val, size_t Rank>
  requires (Rank >= 1)
class Mdspan {
public:

  /// Shape span type.
  using ShapeSpan = std::span<const size_t, Rank>;

  /// Value span type.
  using ValSpan = std::span<Val>;

  /// Initialize the multidimensional span.
  /// @{
  template<std::contiguous_iterator ValIter>
    requires std::constructible_from<ValSpan, ValIter, ValIter>
  constexpr Mdspan(ShapeSpan shape, ValIter val_iter) noexcept
      : shape_{shape}, vals_{val_iter, val_iter + size_from_shape_()} {}
  template<std::ranges::contiguous_range Vals>
    requires std::constructible_from<ValSpan, Vals&&>
  constexpr Mdspan(ShapeSpan shape, Vals&& vals) noexcept
      : shape_{shape}, vals_{std::forward<Vals>(vals)} {
    TIT_ASSERT(vals_.size() == size_from_shape_(), "Data size is invalid!");
  }
  /// @}

  /// Amount of elements.
  constexpr auto size() const noexcept -> size_t {
    return vals_.size();
  }

  /// Iterator pointing to the first span element.
  constexpr auto begin() const noexcept {
    return vals_.begin();
  }
  /// Iterator pointing to the element after the last span element.
  constexpr auto end() const noexcept {
    return vals_.end();
  }

  /// Reference to the first span element.
  constexpr auto front() const noexcept -> Val& {
    return vals_.front();
  }
  /// Reference to the last span element.
  constexpr auto back() const noexcept -> Val& {
    return vals_.back();
  }

  /// Reference to span element or sub-span.
  template<class... Indices>
    requires mdindex<Rank, Indices...>
  constexpr auto operator[](Indices... indices) const noexcept
      -> decltype(auto) {
    // Compute an offset to the data position.
    size_t offset = 0;
    for (const auto index_pack = make_array<Rank, size_t>(indices...);
         const auto [extent, index] : std::views::zip(shape_, index_pack)) {
      TIT_ASSERT(index < extent, "Index is out of range!");
      offset = index + offset * extent;
    }
    TIT_ASSERT(offset < vals_.size(), "Offset is out of range!");
    // Return with result.
    if constexpr (constexpr auto IndicesRank = sizeof...(Indices);
                  IndicesRank == Rank) {
      return vals_[offset];
    } else {
      return tit::Mdspan{shape_.template subspan<IndicesRank>(),
                         vals_.begin() + offset};
    }
  }

private:

  constexpr auto size_from_shape_() const noexcept -> size_t {
    auto s = shape_[0];
    for (size_t i = 1; i < Rank; ++i) s *= shape_[i];
    return s;
  }

  ShapeSpan shape_;
  ValSpan vals_;

}; // class Mdspan

template<contiguous_fixed_size_range Shape, std::contiguous_iterator ValIter>
Mdspan(Shape&&, ValIter)
    -> Mdspan<std::remove_reference_t<std::iter_reference_t<ValIter>>,
              range_fixed_size_v<Shape>>;

template<contiguous_fixed_size_range Shape, std::ranges::contiguous_range Vals>
Mdspan(Shape&&, Vals&&)
    -> Mdspan<std::remove_reference_t<std::ranges::range_reference_t<Vals&&>>,
              range_fixed_size_v<Shape>>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Basic multidimensional owning container.
template<class Val, size_t Rank>
  requires (Rank >= 1)
class Mdvector {
public:

  /// Construct multidimensional vector.
  constexpr Mdvector() noexcept = default;

  /// Construct multidimensional vector with specified size.
  template<class... Extents>
    requires mdshape<Rank, Extents...>
  constexpr explicit Mdvector(Extents... extents) {
    assign(extents...);
  }

  /// Vector size.
  constexpr auto size() const noexcept -> size_t {
    return vals_.size();
  }

  /// Iterator pointing to the first vector element.
  /// @{
  constexpr auto begin() noexcept {
    return vals_.begin();
  }
  constexpr auto begin() const noexcept {
    return vals_.begin();
  }
  /// @}

  /// Iterator pointing to the element after the last vector element.
  /// @{
  constexpr auto end() noexcept {
    return vals_.end();
  }
  constexpr auto end() const noexcept {
    return vals_.end();
  }
  /// @}

  /// Reference to the first span element.
  /// @{
  constexpr auto front() noexcept -> Val& {
    return vals_.front();
  }
  constexpr auto front() const noexcept -> const Val& {
    return vals_.front();
  }
  /// @}

  /// Reference to the last span element.
  /// @{
  constexpr auto back() noexcept -> Val& {
    return vals_.back();
  }
  constexpr auto back() const noexcept -> const Val& {
    return vals_.back();
  }
  /// @}

  /// Clear the vector.
  constexpr void clear() noexcept {
    shape_.fill(0);
    vals_.clear();
  }

  /// Clear and reshape the vector.
  template<class... Extents>
    requires mdshape<Rank, Extents...>
  constexpr void assign(Extents... extents) {
    shape_ = {static_cast<size_t>(extents)...};
    vals_.clear(), vals_.resize((extents * ...));
  }

  /// Reference to vector element or sub-vector span.
  /// @{
  template<class... Indices>
    requires mdindex<Rank, Indices...>
  constexpr auto operator[](Indices... indices) noexcept -> decltype(auto) {
    return Mdspan{shape_, vals_}[indices...];
  }
  template<class... Indices>
    requires mdindex<Rank, Indices...>
  constexpr auto operator[](Indices... indices) const noexcept
      -> decltype(auto) {
    return Mdspan{shape_, vals_}[indices...];
  }
  /// @}

private:

  std::array<size_t, Rank> shape_{};
  std::vector<Val> vals_;

}; // class Mdvector

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
