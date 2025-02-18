/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <functional>
#include <string>
#include <utility>

#include "tit/core/missing.hpp" // IWYU pragma: keep
#include "tit/core/str_utils.hpp"

#include "tit/py/object.hpp"
#include "tit/py/sequence.hpp"

namespace tit::py {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Python mapping reference.
class Mapping : public Object {
public:

  using Object::at;
  using Object::del;
  using Object::set_at;
  using Object::operator[];

  /// Get the type name of the `Mapping` protocol.
  static consteval auto type_name() -> CStrView {
    return "Mapping";
  }

  /// Check if the object implements the mapping protocol.
  static auto isinstance(const Object& obj) -> bool;

  /// Check if the mapping contains the given key.
  /// @{
  auto has_key(const Object& key) const -> bool;
  auto has_key(CStrView key) const -> bool;
  /// @}

  /// Access the item with the given key.
  /// @{
  auto at(CStrView key) const -> Object;
  void set_at(CStrView key, const Object& value) const;
  template<not_object Value>
  void set_at(CStrView key, Value&& value) const {
    set_at(key, object(std::forward<Value>(value)));
  }
  auto operator[](CStrView key) const {
    return ItemAt<Mapping, std::string>{*this, std::string{key}};
  }
  /// @}

  /// Delete the item with the given key.
  void del(CStrView key) const;

  /// Keys of the mapping.
  auto keys() const -> List;

  /// Values of the mapping.
  auto values() const -> List;

  /// Items of the mapping.
  auto items() const -> List;

protected:

  /// Construct a new reference to the existing mapping object.
  explicit Mapping(PyObject* ptr);

}; // class Mapping

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Reference to a Python dictionary.
class Dict final : public Mapping {
public:

  /// Get the type object of the `Dict`.
  static auto type() -> Type;

  /// Check if the object is a subclass of `Dict`.
  static auto isinstance(const Object& obj) -> bool;

  /// Construct a new empty dictionary.
  explicit Dict();

  /// Construct a dictionary object from Python object, similar to `dict(obj)`.
  explicit Dict(const Object& mapping_or_iterable);

  /// Clear the dict.
  void clear() const;

  /// Update the dict with the other mapping or iterable of key-value pairs.
  void update(const Object& mapping_or_iterable) const;

  /// Iterate over the dictionary.
  void for_each(
      std::move_only_function<void(const Object&, const Object&)> func) const;

}; // class Dict

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Reference to a Python set.
class Set final : public Object {
public:

  /// Get the type object of the `Set`.
  static auto type() -> Type;

  /// Check if the object is a subclass of `Set`.
  static auto isinstance(const Object& obj) -> bool;

  /// Construct a new empty set object.
  explicit Set();

  /// Construct a set object from Python object, similar to `set(obj)`.
  explicit Set(const Object& iterable);

  /// Clear the set.
  void clear() const;

  /// Check if the set contains the given item.
  /// @{
  auto has(const Object& value) const -> bool;
  template<not_object Value>
  auto has(Value&& value) const -> bool {
    return has(object(std::forward<Value>(value)));
  }
  /// @}

  /// Add the item to the set.
  /// @{
  void add(const Object& value) const;
  template<not_object Value>
  void add(Value&& value) const {
    add(object(std::forward<Value>(value)));
  }
  /// @}

  /// Remove the item from the set.
  /// @{
  void discard(const Object& value) const;
  template<not_object Value>
  void discard(Value&& value) const {
    discard(object(std::forward<Value>(value)));
  }
  /// @}

  /// Pop an item from the set.
  auto pop() const -> Object;

}; // class Set

/// Construct a new set object containing the given items.
template<class... Values>
auto make_set(Values&&... values) -> Set {
  Set result;
  (result.add(std::forward<Values>(values)), ...);
  return result;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::py
