/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <bit>
#include <concepts>
#include <cstddef>
#include <span>
#include <utility>
#include <vector>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/simd.hpp"

#include "tit/data/dtype.hpp"

namespace tit::data {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Data array.
class DataArray final {
public:

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Construct an empty data array with the given data type @p dtype.
  constexpr explicit DataArray(Dtype dtype) noexcept : dtype_{dtype} {
    TIT_ASSERT(dtype_.known(), "Unknown data type!");
  }

  /// Construct a data array containing @p count copies of a zero initialzied
  /// elements of the data type @p dtype.
  constexpr DataArray(size_t count, Dtype dtype) : DataArray{dtype} {
    resize(count);
  }

  /// Construct a data array containing @p count copies of a @p val.
  template<class Val>
    requires (!std::same_as<Val, Dtype>)
  constexpr DataArray(size_t count, const Val& val)
      : DataArray{count, dtype_of_v<Val>} {
    for (auto& elem : this->all<Val>()) std::construct_at(&elem, val);
  }

  /// Construct a data array with values.
  template<class Val, class... RestVals>
    requires (std::same_as<Val, RestVals> && ...) &&
             (!std::same_as<Val, Dtype>) &&
             ((!std::same_as<RestVals, Dtype>) && ...)
  constexpr DataArray(Val val, RestVals... rest_vals)
      : DataArray{dtype_of_v<Val>} {
    push_back(std::move(val));
    (push_back(std::move(rest_vals)), ...);
  }

  /// Data type specification.
  constexpr auto dtype() const noexcept -> Dtype {
    return dtype_;
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  //
  // Element access
  //

  /// Get a span of typed data array elements.
  /// @{
  template<class Val>
  constexpr auto all() noexcept -> std::span<Val> {
    TIT_ASSERT(dtype_of_v<Val> == dtype_, "Types mismatch!");
    return std::span{std::bit_cast<Val*>(data_.data()), size()};
  }
  template<class Val>
  constexpr auto all() const noexcept -> std::span<const Val> {
    TIT_ASSERT(dtype_of_v<Val> == dtype_, "Types mismatch!");
    return std::span{std::bit_cast<const Val*>(data_.data()), size()};
  }
  /// @}

  /// Get the data array element at index.
  /// @{
  template<class Val>
  constexpr auto at(size_t index) -> Val& {
    TIT_ASSERT(index < size(), "Index is out of range!");
    return this->all<Val>()[index];
  }
  template<class Val>
  constexpr auto at(size_t index) const -> const Val& {
    TIT_ASSERT(index < size(), "Index is out of range!");
    return this->all<Val>()[index];
  }
  /// @}

  /// Get the first data array element.
  /// @{
  template<class Val>
  constexpr auto front() noexcept -> Val& {
    TIT_ASSERT(!empty(), "Data array is empty!");
    return this->all<Val>().front();
  }
  template<class Val>
  constexpr auto front() const noexcept -> const Val& {
    TIT_ASSERT(!empty(), "Data array is empty!");
    return this->all<Val>().front();
  }
  /// @}

  /// Get the last data array element.
  /// @{
  template<class Val>
  constexpr auto back() noexcept -> Val& {
    TIT_ASSERT(!empty(), "Data array is empty!");
    return this->all<Val>().back();
  }
  template<class Val>
  constexpr auto back() const noexcept -> const Val& {
    TIT_ASSERT(!empty(), "Data array is empty!");
    return this->all<Val>().back();
  }
  /// @}

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  //
  // Capacity
  //

  /// Is the data array empty?
  constexpr auto empty() const noexcept -> bool {
    return data_.empty();
  }

  /// Size of the data array.
  constexpr auto size() const noexcept -> size_t {
    return data_.size() / dtype_.size();
  }

  /// Maximum size of the data array.
  constexpr auto max_size() const noexcept -> size_t {
    return data_.max_size() / dtype_.size();
  }

  /// Reserve storage for the data array.
  constexpr void reserve(size_t capacity) {
    data_.reserve(capacity * dtype_.size());
  }

  /// Capacity of the data array.
  constexpr auto capacity() const noexcept -> size_t {
    return data_.capacity() / dtype_.size();
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  //
  // Modifiers
  //

  /// Clear the data array.
  constexpr void clear() noexcept {
    data_.clear();
  }

  /// @todo `emplace` is missing.

  /// Construct an element in-place at the end of the data array.
  template<class Val, class... Args>
    requires std::constructible_from<Val, Args&&...>
  constexpr auto emplace_back(Args&&... args) -> Val& {
    TIT_ASSERT(dtype_of_v<Val> == dtype_, "Types mismatch!");
    data_.resize(data_.size() + sizeof(Val));
    return *std::construct_at(&this->all<Val>().back(),
                              std::forward<Args>(args)...);
  }

  /// @todo `insert` is missing.

  /// @todo `insert_range` is missing.

  /// Adds an element to the end of the data array.
  template<class Val>
  constexpr auto push_back(Val val) -> Val& {
    return this->emplace_back<Val>(std::move(val));
  }

  /// @todo `erase` is missing.

  /// Remove the last element of the data array.
  constexpr auto pop_back() noexcept {
    data_.resize(data_.size() - dtype_.size());
  }

  /// Change the number of elements stored in the data array.
  constexpr void resize(size_t new_size) {
    data_.resize(new_size * dtype_.size());
  }

  /// Swap the data array contents.
  constexpr void swap(DataArray& other) noexcept {
    std::swap(data_, other.data_);
    std::swap(dtype_, other.dtype_);
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

private:

  Dtype dtype_;
  std::vector<std::byte, simd::Allocator<std::byte>> data_;

}; // class DataArray

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::data
