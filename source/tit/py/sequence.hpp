/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <span>
#include <string_view>
#include <utility>
#include <vector>

#include "tit/core/basic_types.hpp"
#include "tit/core/str_utils.hpp"
#include "tit/core/tuple_utils.hpp"

#include "tit/py/object.hpp"

namespace tit::py {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Python sequence reference.
class Sequence : public Object {
public:

  using Object::at;
  using Object::del;
  using Object::set_at;
  using Object::operator[];

  /// Get the type name of the `Sequence` protocol.
  static consteval auto type_name() -> CStrView {
    return "Sequence";
  }

  /// Check if the object implements the sequence protocol.
  static auto isinstance(const Object& obj) -> bool;

  /// Access an item in the sequence.
  /// @{
  auto at(size_t index) const -> Object;
  void set_at(size_t index, const Object& value) const;
  template<not_object Value>
  void set_at(size_t index, Value&& value) const {
    set_at(index, object(std::forward<Value>(value)));
  }
  auto operator[](size_t index) const -> ItemAt<Sequence, size_t> {
    return {*this, index};
  }
  /// @}

  /// Access a slice of the sequence.
  /// @{
  auto at(pair_of_t<size_t> slice) const -> Sequence;
  void set_at(pair_of_t<size_t> slice, const Object& value) const;
  template<not_object Value>
  void set_at(pair_of_t<size_t> slice, Value&& values) const {
    set_at(slice, object(std::forward<Value>(values)));
  }
  auto operator[](pair_of_t<size_t> slice) const
      -> ItemAt<Sequence, pair_of_t<size_t>> {
    return {*this, slice};
  }
  /// @}

  /// Delete the item.
  void del(size_t index) const;

  /// Delete the slice.
  void del(pair_of_t<size_t> slice) const;

  /// Count the number of occurrences of the item in the sequence.
  /// @{
  auto count(const Object& value) const -> size_t;
  template<not_object Value>
  auto count(Value&& value) const -> size_t {
    return count(object(std::forward<Value>(value)));
  }
  /// @}

  /// Check that the sequence contains the given item.
  /// @{
  auto contains(const Object& value) const -> bool;
  template<not_object Value>
  auto contains(Value&& value) const -> bool {
    return contains(object(std::forward<Value>(value)));
  }
  /// @}

  /// Find the index of the item in the sequence.
  /// @{
  auto index(const Object& value) const -> size_t;
  template<not_object Value>
  auto index(Value&& value) const -> size_t {
    return index(object(std::forward<Value>(value)));
  }
  /// @}

  /// Repeat the sequence.
  /// @{
  friend auto operator*(size_t n, const Sequence& a) -> Sequence;
  friend auto operator*(const Sequence& a, size_t n) -> Sequence;
  friend auto operator*=(Sequence& a, size_t n) -> Sequence&;
  /// @}

protected:

  /// Construct a new reference to the existing sequence object.
  explicit Sequence(PyObject* ptr);

}; // class Sequence

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Python string reference.
class Str final : public Sequence {
public:

  /// Get the type object of the `Str`.
  static auto type() -> Type;

  /// Check if the object is a subclass of `Str`.
  static auto isinstance(const Object& obj) -> bool;

  /// Construct a string object from C++ string.
  explicit Str(std::string_view str = "");

  /// Construct a string object from Python object, similar to `str(obj)`.
  explicit Str(const Object& obj);

  /// Get the C++ string.
  auto val() const -> CStrView;

}; // class Str

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Reference to a Python tuple.
class Tuple final : public Sequence {
public:

  /// Get the type object of the `Tuple`.
  static auto type() -> Type;

  /// Check if the object is a subclass of `Tuple`.
  static auto isinstance(const Object& obj) -> bool;

  /// Construct a new empty tuple.
  explicit Tuple();

  /// Construct a tuple object from Python object, similar to `tuple(obj)`.
  explicit Tuple(const Object& iterable);

}; // class Tuple

/// Construct a new tuple object containing the given items.
/// @{
auto to_tuple(std::span<const Object> values) -> Tuple;
template<class... Values>
auto make_tuple(Values&&... values) -> Tuple {
  /// @todo In C++26 `std::span` can be constructed from a braced-init-list.
  return to_tuple(std::vector<Object>{object(std::forward<Values>(values))...});
}
/// @}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Python list reference.
class List final : public Sequence {
public:

  /// Get the type object of the `List`.
  static auto type() -> Type;

  /// Check if the object is a subclass of `List`.
  static auto isinstance(const Object& obj) -> bool;

  /// Construct a new empty list.
  explicit List();

  /// Construct a list object from Python object, similar to `list(obj)`.
  explicit List(const Object& iterable);

  /// Insert an item to the list.
  /// @{
  void insert(size_t index, const Object& value) const;
  template<not_object Value>
  void insert(size_t index, Value&& value) const {
    insert(index, object(std::forward<Value>(value)));
  }
  /// @}

  /// Append an item to the list.
  /// @{
  void append(const Object& value) const;
  template<not_object Value>
  void append(Value&& value) const {
    append(object(std::forward<Value>(value)));
  }
  /// @}

  /// Sort the list.
  void sort() const;

  /// Reverse the list.
  void reverse() const;

}; // class List

/// Construct a new list object containing the given items.
/// @{
auto to_list(std::span<const Object> values) -> List;
template<class... Values>
auto make_list(Values&&... values) -> List {
  /// @todo In C++26 `std::span` can be constructed from a braced-init-list.
  return to_list(std::vector<Object>{object(std::forward<Values>(values))...});
}
/// @}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::py
