/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/// @todo Tests are missing.
#pragma once

#include <initializer_list>
#include <iterator>
#include <memory>
#include <ranges>
#include <type_traits>
#include <utility>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/containers/utils.hpp"
#include "tit/core/range.hpp"
#include "tit/core/utils.hpp"

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Dynamic fixed-size array. Like `std::vector`, but cannot grow or shrink.
/// Unlike `std::vector`, stores only a data pointer and size. Mostly used as
/// a temporary container / building block for other containers.
template<class Item>
  requires std::is_object_v<Item> && std::is_destructible_v<Item>
class FixedArray final {
public:

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Construct an empty array.
  constexpr FixedArray() noexcept = default;

  /// Move-construct an array.
  constexpr FixedArray(FixedArray&& other) noexcept
      : data_{std::exchange(other.data_, nullptr)},
        size_{std::exchange(other.size_, 0)} {}

  /// Copy-construct an array.
  constexpr FixedArray(const FixedArray& other) {
    *this = other;
  }

  /// Construct an array with given size and initial value.
  constexpr explicit FixedArray(size_t size, const Item& init = Item{}) {
    assign(size, init);
  }

  /// Construct an array from an iterator and a sentinel.
  template<iterator_compatible_with<Item> Iter,
           std::sized_sentinel_for<Iter> Sent>
  constexpr explicit FixedArray(Iter first, Sent last) {
    assign(first, last);
  }

  /// Construct an array from an initializer list.
  constexpr FixedArray(std::initializer_list<Item> init) {
    assign(init);
  }

  /// Construct an array from a range.
  template<range_compatible_with<Item> Range>
    requires std::ranges::sized_range<Range>
  constexpr explicit FixedArray(std::from_range_t /*tag*/, Range&& range) {
    assign_range(std::forward<Range>(range));
  }

  /// Destruct an array.
  constexpr ~FixedArray() noexcept {
    if (data_ == nullptr) return;
    TIT_ASSERT(size_ > 0, "Array must not be empty!");
    std::ranges::destroy(*this);
    default_allocator<Item>().deallocate(data_, size_);
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Move-assign an array.
  constexpr auto operator=(FixedArray&& other) noexcept -> FixedArray& {
    if (this != &other) {
      this->~FixedArray();
      data_ = std::exchange(other.data_, nullptr);
      size_ = std::exchange(other.size_, 0);
    }
    return *this;
  }

  /// Copy-assign an array.
  constexpr auto operator=(const FixedArray& other) -> FixedArray& {
    if (this != &other) assign_range(other);
    return *this;
  }

  /// Assign the initializer list to the array.
  constexpr auto operator=(std::initializer_list<Item> init) -> FixedArray& {
    assign(init);
    return *this;
  }

  /// Reset the array to new size and initial value.
  constexpr void assign(size_t size, const Item& init = Item{}) {
    assign_range(std::views::repeat(init, size));
  }

  /// Assign the iterator range to the array.
  template<iterator_compatible_with<Item> Iter,
           std::sized_sentinel_for<Iter> Sent>
  constexpr void assign(Iter first, Sent last) {
    assign_range(std::ranges::subrange{first, last});
  }

  /// Assign the initializer list to the array.
  constexpr void assign(std::initializer_list<Item> init) {
    assign_range(init);
  }

  /// Assign the range to the array.
  template<range_compatible_with<Item> Range>
    requires std::ranges::sized_range<Range>
  constexpr void assign_range(Range&& range) {
    this->~FixedArray();
    size_ = std::ranges::size(range);
    data_ = size_ > 0 ? default_allocator<Item>().allocate(size_) : nullptr;
    std::ranges::uninitialized_copy(std::forward<Range>(range), *this);
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Array size.
  constexpr auto size() const noexcept -> size_t {
    return size_;
  }

  /// Is array empty?
  constexpr auto empty() const noexcept -> bool {
    return size() == 0;
  }

  /// Array data.
  constexpr auto data(this auto&& self) noexcept -> auto* {
    return std::addressof(TIT_FORWARD_LIKE(self, *self.data_));
  }

  /// Access the first item.
  constexpr auto front(this auto&& self) noexcept -> auto&& {
    TIT_ASSERT(!self.empty(), "Array is empty!");
    return TIT_FORWARD_LIKE(self, self.data_[0]);
  }

  /// Access the last item.
  constexpr auto back(this auto&& self) noexcept -> auto&& {
    TIT_ASSERT(!self.empty(), "Array is empty!");
    return TIT_FORWARD_LIKE(self, self.data_[self.size_ - 1]);
  }

  /// Access item at index.
  constexpr auto operator[](this auto&& self, size_t i) noexcept -> auto&& {
    TIT_ASSERT(i < self.size(), "Index is out of range!");
    return TIT_FORWARD_LIKE(self, self.data_[i]);
  }

  /// Iterator pointing to the first item.
  constexpr auto begin(this auto& self) noexcept {
    return RandomAccessIterator{self};
  }

  /// Iterator pointing after the last item.
  constexpr auto end(this auto& self) noexcept {
    return RandomAccessIterator{self, self.size()};
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

private:

  Item* data_ = nullptr;
  size_t size_ = 0;

}; // class FixedArray

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

template<std::input_iterator Iter, std::sized_sentinel_for<Iter> Sent>
FixedArray(Iter first, Sent last) -> FixedArray<std::iter_value_t<Iter>>;

template<std::ranges::input_range Range>
  requires std::ranges::sized_range<Range>
FixedArray(std::from_range_t, Range&& range)
    -> FixedArray<std::ranges::range_value_t<Range>>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
