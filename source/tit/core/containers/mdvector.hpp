/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
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

namespace impl {
template<size_t Rank>
constexpr auto size_from_shape(std::span<const size_t, Rank> shape) noexcept
    -> size_t {
  auto s = shape[0];
  for (size_t i = 1; i < Rank; ++i) s *= shape[i];
  return s;
}
} // namespace impl

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
      : shape_{shape},
        vals_{val_iter, val_iter + impl::size_from_shape(shape)} {}
  template<std::ranges::contiguous_range Vals>
    requires std::constructible_from<ValSpan, Vals&&>
  constexpr Mdspan(ShapeSpan shape, Vals&& vals) noexcept
      : shape_{shape}, vals_{std::forward<Vals>(vals)} {
    TIT_ASSERT(vals_.size() == impl::size_from_shape(shape_),
               "Data size is invalid!");
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
    requires mdindex<Rank, const Indices&...>
  constexpr auto operator[](const Indices&... indices) const noexcept
      -> decltype(auto) {
    // Compute an offset to the data position.
    size_t offset = 0;
    for (const auto index_pack = make_array<Rank, size_t>(indices...);
         const auto& [extent, index] : std::views::zip(shape_, index_pack)) {
      TIT_ASSERT(index < extent, "Index is out of range!");
      offset = index + offset * extent;
    }
    TIT_ASSERT(offset < vals_.size(), "Offset is out of range!");
    // Return with result.
    if constexpr (constexpr auto IndicesRank = packed_array_size_v<Indices...>;
                  IndicesRank == Rank) {
      return vals_[offset];
    } else {
      return tit::Mdspan{shape_.template subspan<IndicesRank>(),
                         vals_.begin() + offset};
    }
  }

private:

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
  constexpr explicit Mdvector(const Extents&... extents) {
    assign(extents...);
  }

  /// Vector size.
  constexpr auto size() const noexcept -> size_t {
    return vals_.size();
  }

  /// Iterator pointing to the first vector element.
  constexpr auto begin(this auto& self) noexcept {
    return self.vals_.begin();
  }

  /// Iterator pointing to the element after the last vector element.
  constexpr auto end(this auto& self) noexcept {
    return self.vals_.end();
  }

  /// Reference to the first span element.
  constexpr auto front(this auto&& self) noexcept -> auto&& {
    return std::forward_like<decltype(self)>(self.vals_.front());
  }

  /// Reference to the last span element.
  constexpr auto back(this auto&& self) noexcept -> auto&& {
    return std::forward_like<decltype(self)>(self.vals_.back());
  }

  /// Clear the vector.
  constexpr void clear() noexcept {
    shape_.fill(0);
    vals_.clear();
  }

  /// Clear and reshape the vector.
  template<class... Extents>
    requires mdshape<Rank, const Extents&...>
  constexpr void assign(const Extents&... extents) {
    shape_ = make_array<Rank, size_t>(extents...);
    vals_.clear();
    vals_.resize(impl::size_from_shape(std::span<const size_t, Rank>{shape_}));
  }

  /// Reference to vector element or sub-vector span.
  template<class... Indices>
    requires mdindex<Rank, Indices...>
  constexpr auto operator[](this auto& self, Indices... indices) noexcept
      -> decltype(auto) {
    return Mdspan{self.shape_, self.vals_}[indices...];
  }

private:

  std::array<size_t, Rank> shape_{};
  std::vector<Val> vals_;

}; // class Mdvector

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit

// Enable borrowed range support for Mdspan, allowing it to be used safely
// in range expressions where the range must outlive the operation.
template<class Val, tit::size_t Rank>
constexpr bool std::ranges::enable_borrowed_range<tit::Mdspan<Val, Rank>> =
    true;
