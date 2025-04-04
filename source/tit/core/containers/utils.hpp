/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/// @todo Tests are missing.
#pragma once

#include <iterator>
#include <memory>
#include <ranges>
#include <type_traits>
#include <utility>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Random access container.
template<class Container>
concept random_access_container =
    requires (Container& container, size_t index) {
      { container[index] };
    };

/// Contiguous container.
template<class Container>
concept contiguous_container =
    random_access_container<Container> && requires (Container& container) {
      { container.data() };
    };

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Iterator for a container that supports random access.
template<random_access_container RandomAccessContainer>
class RandomAccessIterator final {
public:

  using reference_type = decltype(std::declval<RandomAccessContainer&>()[0]);
  using value_type = std::remove_cvref_t<reference_type>;
  using difference_type = typename decltype( //
      std::declval<std::ranges::iota_view<size_t>>().begin())::difference_type;
  using iterator_category =
      std::conditional_t<contiguous_container<RandomAccessContainer>,
                         std::contiguous_iterator_tag,
                         std::random_access_iterator_tag>;

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Construct an invalid iterator.
  constexpr RandomAccessIterator() noexcept = default;

  /// Construct from a container and an index.
  constexpr explicit RandomAccessIterator(RandomAccessContainer& container,
                                          size_t index = 0) noexcept
      : container_{&container}, index_{index} {}

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Dereference the iterator.
  /// @{
  constexpr auto operator[](size_t offset) const noexcept -> reference_type {
    TIT_ASSERT(container_ != nullptr, "Iterator is not dereferenceable!");
    return (*container_)[index_ + offset];
  }
  constexpr auto operator*() const noexcept -> reference_type {
    return (*this)[0];
  }
  constexpr auto operator->() const noexcept
      -> std::add_pointer_t<reference_type> {
    return std::addressof(**this);
  }
  /// @}

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Increment the iterator.
  /// @{
  constexpr auto operator++() noexcept -> RandomAccessIterator& {
    ++index_;
    return *this;
  }
  constexpr auto operator++(int) noexcept -> RandomAccessIterator {
    return RandomAccessIterator{*container_, index_++};
  }
  /// @}

  /// Decrement the iterator.
  /// @{
  constexpr auto operator--() noexcept -> RandomAccessIterator& {
    --index_;
    return *this;
  }
  constexpr auto operator--(int) noexcept -> RandomAccessIterator {
    return RandomAccessIterator{*container_, index_--};
  }
  /// @}

  /// Add an offset to the iterator.
  /// @{
  friend constexpr auto operator+(RandomAccessIterator iter,
                                  ssize_t offset) noexcept
      -> RandomAccessIterator {
    return RandomAccessIterator{*iter.container_, iter.index_ + offset};
  }
  friend constexpr auto operator+(ssize_t offset,
                                  RandomAccessIterator iter) noexcept
      -> RandomAccessIterator {
    return iter + offset;
  }
  friend constexpr auto operator+=(RandomAccessIterator& iter,
                                   ssize_t offset) noexcept
      -> RandomAccessIterator& {
    iter.index_ += offset;
    return iter;
  }
  /// @}

  /// Subtract an offset from the iterator.
  /// @{
  friend constexpr auto operator-(RandomAccessIterator iter,
                                  ssize_t offset) noexcept
      -> RandomAccessIterator {
    return RandomAccessIterator{*iter.container_, iter.index_ + offset};
  }
  friend constexpr auto operator-(ssize_t offset,
                                  RandomAccessIterator iter) noexcept
      -> RandomAccessIterator {
    return iter + offset;
  }
  friend constexpr auto operator-=(RandomAccessIterator& iter,
                                   ssize_t offset) noexcept
      -> RandomAccessIterator& {
    iter.index_ -= offset;
    return iter;
  }
  /// @}

  /// Difference between two iterators.
  friend constexpr auto operator-(RandomAccessIterator lhs,
                                  RandomAccessIterator rhs) -> difference_type {
    TIT_ASSERT(lhs.container_ == rhs.container_,
               "Iterators must be from the same container!");
    return lhs.index_ - rhs.index_;
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Compare two iterators.
  /// @{
  friend constexpr auto operator==(RandomAccessIterator lhs,
                                   RandomAccessIterator rhs) noexcept {
    TIT_ASSERT(lhs.container_ == rhs.container_,
               "Iterators must be from the same container!");
    return lhs.index_ == rhs.index_;
  }
  friend constexpr auto operator<=>(RandomAccessIterator lhs,
                                    RandomAccessIterator rhs) noexcept {
    TIT_ASSERT(lhs.container_ == rhs.container_,
               "Iterators must be from the same container!");
    return lhs.index_ <=> rhs.index_;
  }
  /// @}

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

private:

  RandomAccessContainer* container_ = nullptr;
  size_t index_ = 0;
}; // class RandomAccessIterator

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Default allocator for items.
template<class Item>
constexpr auto default_allocator() noexcept -> std::allocator<Item>& {
  static std::allocator<Item> allocator;
  return allocator;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
