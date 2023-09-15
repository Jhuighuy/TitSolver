/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License
 * See /LICENSE.md for license information.
 * SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <array>
#include <utility>
#include <vector>

#include <tit/core/assert.hpp>
#include <tit/core/types.hpp>

namespace tit {

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/******************************************************************************\
 ** Poor man's multidimensional non-owning container.
\******************************************************************************/
template<class Val, size_t Rank>
class Mdspan {
private:

  Val* vals_;
  const size_t* shape_;

public:

  /** Construct multidimensional span. */
  constexpr Mdspan(Val* vals, const size_t* shape) noexcept
      : vals_{vals}, shape_{shape} {
    TIT_ASSERT(vals_ != nullptr, "Data pointer must be valid.");
    TIT_ASSERT(shape_ != nullptr, "Shape pointer must be valid.");
  }

  /** Span size. */
  constexpr auto size() const noexcept -> size_t {
    // This mess is used to avoid any loops generation.
    return [this]<size_t... is>(std::index_sequence<is...>) {
      return (shape_[is] * ...);
    }(std::make_index_sequence<Rank>{});
  }

  /** Span data. */
  /** @{ */
  constexpr auto data() noexcept -> Val* {
    return vals_;
  }
  constexpr auto data() const noexcept -> const Val* {
    return vals_;
  }
  /** @} */

  /** Iterator pointing to the first span element. */
  constexpr auto begin() const noexcept -> Val* {
    return vals_;
  }
  /** Iterator pointing to the element after the last span element. */
  constexpr auto end() const noexcept -> Val* {
    return vals_ + size();
  }

  /** Reference to the first span element. */
  constexpr auto front() const noexcept -> Val& {
    return vals_[0];
  }
  /** Reference to the last span element. */
  constexpr auto back() const noexcept -> Val& {
    return vals_[size() - 1];
  }

  /** Data offset at indices. */
  template<class... Indices>
    requires (sizeof...(Indices) <= Rank)
  constexpr auto offset(Indices... indices) const noexcept -> size_t {
    size_t offset = 0;
    if constexpr (sizeof...(indices) != 0) {
      // Pack all the indices into an array (and pad it with zeroes).
      const auto mdindex =
          std::array<size_t, Rank>{static_cast<size_t>(indices)...};
      TIT_ASSERT(mdindex[0] < shape_[0], "Index is out of range.");
      offset += mdindex[0];
      for (size_t i = 1; i < Rank; ++i) {
        TIT_ASSERT(mdindex[i] < shape_[i], "Index is out of range.");
        offset = mdindex[i] + offset * shape_[i];
      }
    }
    return offset;
  }

  /** Reference to span element or sub-span. */
  template<class... Indices>
    requires (sizeof...(Indices) <= Rank)
  constexpr auto operator[](Indices... indices) const noexcept
      -> decltype(auto) {
    const auto delta = offset(indices...);
    constexpr auto Size = sizeof...(Indices);
    if constexpr (Size == Rank) return vals_[delta];
    else return Mdspan<Val, Rank - Size>{vals_ + delta, shape_ + Size};
  }

}; // class Mdspan

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/******************************************************************************\
 ** Poor man's multidimensional owning container.
\******************************************************************************/
template<class Val, size_t Rank>
class Mdvector {
private:

  std::vector<Val> vals_;
  std::array<size_t, Rank> shape_{};

  constexpr auto span_() noexcept {
    return Mdspan<Val, Rank>{vals_.data(), shape_.data()};
  }
  constexpr auto span_() const noexcept {
    return Mdspan<const Val, Rank>{vals_.data(), shape_.data()};
  }

public:

  /** Construct multidimensional vector. */
  Mdvector() = default;

  /** Construct multidimensional vector with specified size. */
  template<class... Sizes>
    requires (sizeof...(Sizes) == Rank)
  constexpr explicit Mdvector(Sizes... sizes) {
    assign(sizes...);
  }

  /** Vector size. */
  constexpr auto size() const noexcept -> size_t {
    return vals_.size();
  }

  /** Vector data. */
  /** @{ */
  constexpr auto data() noexcept -> Val* {
    return vals_.data();
  }
  constexpr auto data() const noexcept -> const Val* {
    return vals_.data();
  }
  /** @} */

  /** Iterator pointing to the first vector element. */
  /** @{ */
  constexpr auto begin() noexcept {
    return vals_.begin();
  }
  constexpr auto begin() const noexcept {
    return vals_.begin();
  }
  /** @} */

  /** Iterator pointing to the element after the last vector element. */
  /** @{ */
  constexpr auto end() noexcept {
    return vals_.end();
  }
  constexpr auto end() const noexcept {
    return vals_.end();
  }
  /** @} */

  /** Clear the vector. */
  constexpr void clear() noexcept {
    vals_.clear();
    shape_.fill(0);
  }

  /** Clear and reshape the vector. */
  template<class... Sizes>
    requires (sizeof...(Sizes) == Rank)
  constexpr void assign(Sizes... sizes) {
    vals_.clear(), vals_.resize((sizes * ...));
    shape_ = {sizes...};
  }

  /** Data offset at indices. */
  template<class... Indices>
  constexpr auto offset(Indices... indices) const noexcept -> size_t {
    return span_().offset(indices...);
  }

  /** Reference to vector element or sub-vector span. */
  /** @{ */
  template<class... Indices>
    requires (sizeof...(Indices) <= Rank)
  constexpr auto operator[](Indices... indices) noexcept //
      -> decltype(auto) {
    return span_()[indices...];
  }
  template<class... Indices>
    requires (sizeof...(Indices) <= Rank)
  constexpr auto operator[](Indices... indices) const noexcept
      -> decltype(auto) {
    return span_()[indices...];
  }
  /** @} */

}; // class Mdvector

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

} // namespace tit
