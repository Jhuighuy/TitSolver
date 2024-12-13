/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <bit>
#include <concepts>
#include <functional>
#include <ranges>
#include <type_traits>
#include <utility>
#include <vector>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/mat.hpp"
#include "tit/core/vec.hpp"

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Packed array of scalars.
template<class Val>
  requires std::is_object_v<Val>
class PackedArray final {
public:

  /// Construct a packed array from a range of values.
  /// Values would be copied into the array.
  template<std::ranges::input_range Vals>
    requires (std::same_as<std::ranges::range_value_t<Vals>, Val> &&
              !(std::ranges::sized_range<Vals> &&
                std::ranges::contiguous_range<Vals>) )
  constexpr explicit PackedArray(Vals&& vals)
      : data_{std::views::all(
            std::ranges::to<std::vector>(std::forward<Vals>(vals)))} {}

  /// Construct a packed array from a sized contiguous range view of values.
  /// Range view would be owned by the array.
  template<std::ranges::view Vals>
    requires (std::same_as<std::ranges::range_value_t<Vals>, Val> &&
              (std::ranges::sized_range<Vals> &&
               std::ranges::contiguous_range<Vals>) )
  constexpr explicit PackedArray(Vals vals) noexcept
      : data_{std::ranges::data(vals), std::ranges::size(vals)},
        cleanup_{[v = std::move(vals)] {}} {}

  /// Construct a packed array from a sized contiguous range view of bytes.
  /// Range view would be owned by the array.
  template<std::ranges::view Bytes>
    requires (std::same_as<std::ranges::range_value_t<Bytes>, byte_t> &&
              (std::ranges::sized_range<Bytes> &&
               std::ranges::contiguous_range<Bytes>) )
  constexpr explicit PackedArray(Bytes bytes) noexcept
      : data_{std::bit_cast<const Val*>(std::ranges::data(bytes)),
              std::ranges::size(bytes) / sizeof(Val)} {}

  /// Packed array is move-constructible.
  constexpr PackedArray(PackedArray&&) = default;

  /// Packed array is move-assignable.
  constexpr auto operator=(PackedArray&&) -> PackedArray& = default;

  /// Packed array is not copy-constructible.
  PackedArray(const PackedArray&) = delete;

  /// Packed array is not copy-assignable.
  constexpr auto operator=(const PackedArray&) -> PackedArray& = delete;

  /// Packed array is not copyable.
  constexpr auto operator=(PackedArray&) -> PackedArray& = delete;

  /// Destroy the packed array.
  ~PackedArray() noexcept {
    cleanup_();
  }

  /// Number of elements in the array.
  constexpr auto size() const noexcept -> size_t {
    return data_.size();
  }

  /// Get the element at the given index.
  constexpr auto operator[](size_t index) const noexcept -> const Val& {
    TIT_ASSERT(index < size(), "Index is out of range!");
    return data_[index];
  }

  /// Get the bytes of the array.
  constexpr auto bytes() const noexcept -> std::span<const byte_t> {
    return {std::bit_cast<const byte_t*>(data_.data()),
            data_.size() * sizeof(Val)};
  }

private:

  std::span<const Val> data_;
  std::function<void()> cleanup_;

}; // class PackedArray

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Packed array of vectors.
template<class Num, size_t Dim>
class PackedArray<Vec<Num, Dim>> final {
public:

  /// Construct a packed array from a range of vectors.
  template<std::ranges::input_range Vecs>
    requires std::same_as<std::ranges::range_value_t<Vecs>, Vec<Num, Dim>>
  constexpr explicit PackedArray(Vecs&& vecs)
      : elems_{std::forward<Vecs>(vecs) |
               std::views::transform(
                   [](const Vec<Num, Dim>& vec) { return vec.elems(); }) |
               std::views::join} {}

  /// Get the bytes of the array.
  constexpr auto bytes() const noexcept -> std::span<const byte_t> {
    return elems_.bytes();
  }

private:

  PackedArray<Num> elems_;

}; // class PackedArray<Vec>

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Packed array of matrices.
template<class Num, size_t Dim>
class PackedArray<Mat<Num, Dim>> final {
public:

  /// Construct a packed array from a range of matrices.
  template<std::ranges::input_range Mats>
    requires std::same_as<std::ranges::range_value_t<Mats>, Mat<Num, Dim>>
  constexpr explicit PackedArray(Mats&& mats)
      : rows_{std::forward<Mats>(mats) |
              std::views::transform(
                  [](const Mat<Num, Dim>& mat) { return mat.rows(); }) |
              std::views::join} {}

  /// Get the bytes of the array.
  constexpr auto bytes() const noexcept -> std::span<const byte_t> {
    return rows_.bytes();
  }

private:

  PackedArray<Vec<Num, Dim>> rows_;

}; // class PackedArray<Mat>

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

template<std::ranges::range Vals>
  requires (!std::same_as<std::ranges::range_value_t<Vals>, byte_t>)
PackedArray(Vals&&) -> PackedArray<std::ranges::range_value_t<Vals>>;

} // namespace tit
