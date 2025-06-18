/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm>
#include <initializer_list>
#include <iterator>
#include <ranges>
#include <utility>

#include "tit/core/basic_types.hpp"
#include "tit/core/containers/utils.hpp"
#include "tit/core/range.hpp"

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Inplace vector.
template<class Item, size_t Capacity>
class InplaceVector {
public:

  using value_type = Item;
  using size_type = size_t;
  using difference_type = ssize_t;
  using reference = value_type&;
  using const_reference = const value_type&;
  using pointer = value_type*;
  using const_pointer = const value_type*;
  using iterator = RandomAccessIterator<InplaceVector>;
  using const_iterator = RandomAccessIterator<const InplaceVector>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Constructs an empty `InplaceVector` whose `data() == nullptr` and
  // `size() == 0`.
  constexpr InplaceVector() noexcept;

  /// Constructs an `InplaceVector` with count default-inserted elements.
  constexpr explicit InplaceVector(size_type count);

  /// Constructs an `InplaceVector` with `count` copies of elements with
  /// value `value`.
  constexpr InplaceVector(size_type count, const value_type& value);

  /// Constructs an `InplaceVector` with the contents of the
  /// range `[first, last)`.
  template<value_iterator<value_type> InputIter>
  constexpr InplaceVector(InputIter first, InputIter last);

  /// Constructs an `InplaceVector` with the contents of the range `rg`.
  template<value_range<value_type> R>
  constexpr InplaceVector(std::from_range_t, R&& rg);

  /// A copy constructor. Constructs an `InplaceVector` with the copy of the
  /// contents of `other`.
  /// @{
  constexpr InplaceVector(const InplaceVector& other)
    requires (Capacity > 0 &&
              std::is_trivially_copy_constructible_v<value_type>)
  = default;
  constexpr InplaceVector(const InplaceVector& other)
    requires (Capacity == 0 ||
              !std::is_trivially_copy_constructible_v<value_type>);
  /// @}

  /// A move constructor. Constructs an `InplaceVector` with the contents of
  /// `other` using move semantics.
  /// @{
  constexpr InplaceVector(InplaceVector&& other) noexcept(
      Capacity == 0 || std::is_nothrow_move_constructible_v<value_type>)
    requires (Capacity > 0 &&
              std::is_trivially_move_constructible_v<value_type>)
  = default;
  constexpr InplaceVector(InplaceVector&& other) noexcept(
      Capacity == 0 || std::is_nothrow_move_constructible_v<value_type>)
    requires (Capacity == 0 ||
              !std::is_trivially_move_constructible_v<value_type>);
  /// @}

  /// Constructs an `InplaceVector` with the contents of the
  /// initializer list `ilist`.
  constexpr InplaceVector(std::initializer_list<value_type> ilist);

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Destructs the `InplaceVector`. The destructors of the elements are called
  /// and the used storage is deallocated.
  /// @{
  constexpr ~InplaceVector()
    requires (std::is_trivially_destructible_v<value_type>)
  = default;
  constexpr ~InplaceVector()
    requires (!std::is_trivially_destructible_v<value_type>);
  /// @}

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Copy assignment operator.
  /// @{
  constexpr auto operator=(const InplaceVector& other) -> InplaceVector&
    requires (std::is_trivially_destructible_v<value_type> &&
              std::is_trivially_copy_constructible_v<value_type> &&
              std::is_trivially_copy_assignable_v<value_type>)
  = default;
  constexpr auto operator=(const InplaceVector& other) -> InplaceVector&
    requires (!std::is_trivially_destructible_v<value_type> ||
              !std::is_trivially_copy_constructible_v<value_type> ||
              !std::is_trivially_copy_assignable_v<value_type>);
  /// @}

  /// Move assignment operator.
  /// @{
  constexpr auto operator=(InplaceVector&& other) noexcept(
      Capacity == 0 || (std::is_nothrow_move_assignable_v<value_type> &&
                        std::is_nothrow_move_constructible_v<value_type>) )
      -> InplaceVector&
    requires (std::is_trivially_destructible_v<value_type> &&
              std::is_trivially_move_constructible_v<value_type> &&
              std::is_trivially_move_assignable_v<value_type>)
  = default;
  constexpr auto operator=(InplaceVector&& other) noexcept(
      Capacity == 0 || (std::is_nothrow_move_assignable_v<value_type> &&
                        std::is_nothrow_move_constructible_v<value_type>) )
      -> InplaceVector&
    requires (!std::is_trivially_destructible_v<value_type> ||
              !std::is_trivially_move_constructible_v<value_type> ||
              !std::is_trivially_move_assignable_v<value_type>);
  /// @}

  /// Replaces the contents with those identified by initializer list `ilist`.
  constexpr auto operator=(std::initializer_list<value_type> ilist)
      -> InplaceVector&;

  /// Replaces the contents with `count` copies of value `value`.
  constexpr void assign(size_type count, const value_type& value);

  /// Replaces the contents with copies of those in the range `[first, last)`.
  template<value_iterator<value_type> InputIter>
  constexpr void assign(InputIter first, InputIter last);

  /// Replaces the contents with the elements from `ilist`.
  constexpr void assign(std::initializer_list<value_type> ilist);

  /// Replaces elements in the container with a copy of each element in `rg`.
  template<value_range<value_type> R>
  constexpr void assign_range(R&& rg);

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  //
  // Element access
  //

  /// Returns a reference to the element at specified location `pos`, with
  /// bounds checking. If `pos` is not within the range of the container, an
  /// exception of type `std::out_of_range` is thrown.
  /// @{
  constexpr auto at(size_type pos) -> reference;
  constexpr auto at(size_type pos) const -> const_reference;
  /// @}

  /// Returns a reference to the element at specified location `pos`.
  /// @{
  constexpr auto operator[](size_type pos) -> reference;
  constexpr auto operator[](size_type pos) const -> const_reference;
  /// @}

  /// Returns a reference to the first element in the container.
  /// @{
  constexpr auto front() -> reference;
  constexpr auto front() const -> const_reference;
  /// @}

  /// Returns a reference to the last element in the container.
  /// @{
  constexpr auto back() -> reference;
  constexpr auto back() const -> const_reference;
  /// @}

  /// Returns a pointer to the underlying array serving as element storage.
  /// @{
  constexpr auto data() noexcept -> pointer;
  constexpr auto data() const noexcept -> const_pointer;
  /// @}

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  //
  // Iterators
  //

  /// Returns an iterator to the first element of `*this`.
  /// @{
  constexpr auto begin() noexcept -> iterator;
  constexpr auto begin() const noexcept -> const_iterator;
  constexpr auto cbegin() const noexcept -> const_iterator;
  /// @}

  /// Returns an iterator past the last element of `*this`.
  /// @{
  constexpr auto end() noexcept -> iterator;
  constexpr auto end() const noexcept -> const_iterator;
  constexpr auto cend() const noexcept -> const_iterator;
  /// @}

  /// Returns a reverse iterator to the first element of the reversed `*this`.
  /// It corresponds to the last element of the non-reversed `*this`.
  /// @{
  constexpr auto rbegin() noexcept -> reverse_iterator;
  constexpr auto rbegin() const noexcept -> const_reverse_iterator;
  constexpr auto crbegin() const noexcept -> const_reverse_iterator;
  /// @}

  /// Returns a reverse iterator past the last element of the reversed `*this`.
  /// It corresponds to the element preceding the first element of the
  /// non-reversed `*this`.
  /// @{
  constexpr auto rend() noexcept -> reverse_iterator;
  constexpr auto rend() const noexcept -> const_reverse_iterator;
  constexpr auto crend() const noexcept -> const_reverse_iterator;
  /// @}

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  //
  // Size and capacity
  //

  /// Checks if the container has no elements.
  constexpr auto empty() const noexcept -> bool;

  /// Returns the number of elements in the container.
  constexpr auto size() const noexcept -> size_type;

  /// Returns the maximum number of elements the container is able to hold.
  static consteval auto max_size() noexcept -> size_type {
    return Capacity;
  }

  /// Returns the capacity of the internal (inplace) storage.
  static consteval auto capacity() noexcept -> size_type {
    return Capacity;
  }

  /// Resizes the container to contain `count` elements.
  /// @{
  constexpr void resize(size_type count);
  constexpr void resize(size_type count, const value_type& value);
  /// @}

  /// Does nothing, except that may throw `std::bad_alloc`.
  static constexpr void reserve(size_type new_cap) {
    if (new_cap > Capacity) throw std::bad_alloc{};
  }

  /// Does nothing.
  static consteval void shrink_to_fit() noexcept {
    // Do nothing.
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  //
  // Modifiers
  //

  /// Inserts a copy of `value` before `pos`.
  constexpr auto insert(const_iterator pos, const value_type& value)
      -> iterator;

  /// Inserts `value` before `pos`, possibly using move semantics.
  constexpr auto insert(const_iterator pos, value_type&& value) -> iterator;

  /// Inserts count copies of the `value` before `pos`.
  constexpr auto insert(const_iterator pos,
                        size_type count,
                        const value_type& value) -> iterator;

  /// Inserts elements from range `[first, last)` before `pos`.
  template<value_iterator<value_type> InputIter>
  constexpr auto insert(const_iterator pos, InputIter first, InputIter last)
      -> iterator;

  /// Inserts elements from initializer list `ilist` before `pos`.
  constexpr auto insert(const_iterator pos,
                        std::initializer_list<value_type> ilist) -> iterator {
    insert(pos, ilist.begin(), ilist.end());
  }

  /// Inserts, in non-reversing order, copies of elements in `rg` before `pos`.
  template<value_range<value_type> R>
  constexpr auto insert_range(const_iterator pos, R&& rg) -> iterator;

  /// Inserts a new element into the container directly before `pos`. The
  /// arguments `args...` are forwarded to the constructor as
  /// `std::forward<Args>(args)...`.
  template<class... Args>
  constexpr auto emplace(const_iterator position, Args&&... args) -> iterator;

  /// Appends a new element to the end of the container. The arguments `args...`
  /// are forwarded to the constructor as `std::forward<Args>(args)...`.
  /// @{
  template<class... Args>
  constexpr auto unchecked_emplace_back(Args&&... args) -> reference;
  template<class... Args>
  constexpr auto emplace_back(Args&&... args) -> reference;
  /// @}

  /// Conditionally appends an object of type T to the end of the container.
  template<class... Args>
  constexpr auto try_emplace_back(Args&&... args) -> pointer;

  /// Appends the given element `value` to the end of the container.
  /// @{
  constexpr auto unchecked_push_back(const value_type& value) -> reference;
  constexpr auto unchecked_push_back(value_type&& value) -> reference;
  constexpr auto push_back(const value_type& value) -> reference;
  constexpr auto push_back(value_type&& value) -> reference;
  /// @}

  /// Conditionally appends the given element value to the end of the container.
  /// @{
  constexpr auto try_push_back(const value_type& value) -> pointer;
  constexpr auto try_push_back(value_type&& value) -> pointer;
  /// @}

  /// Removes the last element of the container.
  constexpr void pop_back();

  /// Inserts copies of elements from the range `rg` before `end()`, in
  /// non-reversing order.
  template<value_range<value_type> R>
  constexpr void append_range(R&& rg);

  /// Appends copies of initial elements in `rg` before `end()`, until all
  /// elements are inserted or the internal storage is exhausted.
  template<value_range<value_type> R>
  constexpr auto try_append_range(R&& rg)
      -> std::ranges::borrowed_iterator_t<R>;

  /// Erases all elements from the container. After this call, `size()` returns
  /// zero.
  constexpr void clear() noexcept;

  /// Removes the element at `pos`.
  constexpr auto erase(const_iterator pos) -> iterator;

  /// Removes the elements in the range `[first, last)`.
  constexpr auto erase(const_iterator first, const_iterator last) -> iterator;

  /// Exchanges the contents of the container with those of `other`. Does not
  /// cause iterators and references to associate with the other container.
  constexpr void swap(InplaceVector& other) noexcept(
      Capacity == 0 || (std::is_nothrow_swappable_v<value_type> &&
                        std::is_nothrow_move_constructible_v<value_type>) );

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  //
  // Non-member functions
  //

  /// Swaps the contents of `lhs` and `rhs`.
  friend constexpr void swap(InplaceVector& lhs, InplaceVector& rhs) noexcept(
      Capacity == 0 || (std::is_nothrow_swappable_v<value_type> &&
                        std::is_nothrow_move_constructible_v<value_type>) ) {
    lhs.swap(rhs);
  }

  /// Erases all elements that compare equal to `value` from the container `c`.
  template<class Val = Item>
  friend constexpr auto erase(InplaceVector& c, const Val& value) -> size_type {
    const auto iter = std::remove(c.begin(), c.end(), value);
    const auto result = std::distance(iter, c.end());
    c.erase(iter, c.end());
    return result;
  }

  /// Erases all elements that satisfy the predicate `pred` from the
  /// container `c`.
  template<class Pred>
  friend constexpr auto erase_if(InplaceVector& c, Pred pred) -> size_type {
    const auto iter = std::remove_if(c.begin(), c.end(), std::move(pred));
    const auto result = std::distance(iter, c.end());
    c.erase(iter, c.end());
    return result;
  }

  /// Checks if the contents of `lhs` and `rhs` are equal, that is, they have
  /// the same number of elements and each element in `lhs` compares equal with
  /// the element in `rhs` at the same position.
  constexpr friend auto operator==(const InplaceVector& lhs,
                                   const InplaceVector& rhs) -> bool {
    return std::ranges::equal(lhs, rhs);
  }

  /// Compares the contents of `lhs` and `rhs` lexicographically.
  constexpr friend auto operator<=>(const InplaceVector& lhs,
                                    const InplaceVector& rhs) {
    return std::lexicographical_compare_three_way(lhs.begin(),
                                                  lhs.end(),
                                                  rhs.begin(),
                                                  rhs.end());
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

}; // class InplaceVector

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
