/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <array>
#include <concepts> // IWYU pragma: keep
#include <utility>
#include <vector>

#include <tit/core/assert.hpp>
#include <tit/core/misc.hpp>
#include <tit/core/types.hpp>

namespace tit {

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

// TODO: refactor with `std::span`.
// NOLINTBEGIN(*-bounds-pointer-arithmetic)

/******************************************************************************\
 ** @brief Poor man's multidimensional non-owning container.
 ** This class is very basic, since we do not need multidimensional containers
 ** that often.
\******************************************************************************/
template<class Val, size_t Rank>
  requires (Rank >= 1)
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

  /** Reference to span element or sub-span. */
  template<class... Indices>
    requires in_range_v<1UZ, sizeof...(Indices), Rank> &&
             (std::convertible_to<Indices, size_t> && ...)
  constexpr auto operator[](Indices... indices) const noexcept
      -> decltype(auto) {
    // Pack all the indices into an array and pad it with zeroes.
    const auto index_array = pack<Rank, size_t>(indices...);
    // Compute an offset to the data position.
    auto offset = 0UZ;
    for (auto i = 0UZ; i < Rank; ++i) {
      TIT_ASSERT(index_array[i] < shape_[i], "Index is out of range.");
      offset = index_array[i] + offset * shape_[i];
    }
    // Return with result.
    constexpr auto SubspanRank = sizeof...(Indices);
    if constexpr (SubspanRank == Rank) return vals_[offset];
    else {
      return Mdspan<Val, Rank - SubspanRank>{vals_ + offset,
                                             shape_ + SubspanRank};
    }
  }

}; // namespace tit

// NOLINTEND(*-bounds-pointer-arithmetic)

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/******************************************************************************\
 ** @brief Poor man's multidimensional owning container.
 ** This class is very basic, since we do not need multidimensional containers
 ** that often.
\******************************************************************************/
template<class Val, size_t Rank>
  requires (Rank >= 1)
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
    requires (sizeof...(Sizes) == Rank) &&
             (std::convertible_to<Sizes, size_t> && ...)
  constexpr explicit Mdvector(Sizes... sizes) {
    assign(sizes...);
  }

  /** Vector size. */
  constexpr auto size() const noexcept -> size_t {
    return vals_.size();
  }

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

  /** Reference to the first span element. */
  /** @{ */
  constexpr auto front() noexcept -> Val& {
    return vals_.front();
  }
  constexpr auto front() const noexcept -> const Val& {
    return vals_.front();
  }
  /** @} */
  /** Reference to the last span element. */
  /** @{ */
  constexpr auto back() noexcept -> Val& {
    return vals_.back();
  }
  constexpr auto back() const noexcept -> const Val& {
    return vals_.back();
  }
  /** @} */

  /** Clear the vector. */
  constexpr void clear() noexcept {
    vals_.clear();
    shape_.fill(0);
  }

  /** Clear and reshape the vector. */
  template<class... Sizes>
    requires (sizeof...(Sizes) == Rank) &&
             (std::convertible_to<Sizes, size_t> && ...)
  constexpr void assign(Sizes... sizes) {
    vals_.clear(), vals_.resize((sizes * ...));
    shape_ = {static_cast<size_t>(sizes)...};
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
