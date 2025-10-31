/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm>
#include <array>
#include <functional>
#include <iterator>
#include <ranges>
#include <span>
#include <type_traits>
#include <vector>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"

namespace tit {

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
  constexpr auto operator[](const std::array<size_t, Rank>& indices)
      const noexcept -> decltype(auto) {
    size_t offset = 0;
    for (const auto [extent, index] : std::views::zip(shape_, indices)) {
      TIT_ASSERT(index < extent, "Index is out of range!");
      offset = index + offset * extent;
    }
    return vals_[offset];
  }

private:

  std::span<Val> vals_;
  Shape shape_;

}; // class Mdspan

namespace impl {

template<std::ranges::contiguous_range Range>
inline constexpr auto span_extent_v =
    decltype(std::span{std::declval<Range&>()})::extent;

} // namespace impl

template<std::contiguous_iterator ValIter, std::ranges::contiguous_range Shape>
  requires std::ranges::sized_range<Shape> &&
           (impl::span_extent_v<Shape> != std::dynamic_extent)
Mdspan(ValIter, Shape&&)
    -> Mdspan<std::remove_reference_t<std::iter_reference_t<ValIter>>,
              impl::span_extent_v<Shape>>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Basic multidimensional owning container.
template<class Val, size_t Rank>
  requires (Rank >= 1)
class Mdvector final {
public:

  /// Construct an empty multidimensional vector.
  constexpr Mdvector() noexcept = default;

  /// Construct a multidimensional vector with given shape and clear values.
  constexpr explicit Mdvector(const std::array<size_t, Rank>& shape) {
    assign(shape);
  }

  /// Construct a multidimensional vector with given shape and values.
  constexpr explicit Mdvector(std::forward_iterator auto iter,
                              const std::array<size_t, Rank>& shape) {
    assign(iter, shape);
  }

  /// Vector size.
  constexpr auto size() const noexcept -> size_t {
    return vals_.size();
  }

  /// Vector shape.
  constexpr auto shape() const noexcept -> const std::array<size_t, Rank>& {
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
  constexpr void assign(const std::array<size_t, Rank>& shape) {
    shape_ = shape;
    vals_.clear();
    vals_.resize(std::ranges::fold_left(shape_, 1, std::multiplies{}));
  }

  /// Reshape the vector and assign values.
  constexpr void assign(std::forward_iterator auto iter,
                        const std::array<size_t, Rank>& shape) {
    assign(shape);
    std::ranges::copy(iter, iter + size(), vals_.begin());
  }

  /// Reference to vector element or sub-vector span.
  constexpr auto operator[](this auto& self,
                            const std::array<size_t, Rank>& indices) noexcept
      -> decltype(auto) {
    return Mdspan{self.vals_.begin(), self.shape_}[indices];
  }

private:

  std::array<size_t, Rank> shape_{};
  std::vector<Val> vals_;

}; // class Mdvector

template<std::contiguous_iterator ValIter, size_t Rank>
Mdvector(ValIter, const std::array<size_t, Rank>&)
    -> Mdvector<std::iter_value_t<ValIter>, Rank>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit

// Enable borrowed range support for Mdspan, allowing it to be used safely
// in range expressions where the range must outlive the operation.
template<class Val, tit::size_t Rank>
constexpr bool std::ranges::enable_borrowed_range<tit::Mdspan<Val, Rank>> =
    true;
