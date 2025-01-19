/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <array>
#include <concepts>
#include <span>
#include <string>
#include <type_traits>
#include <utility>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/str_utils.hpp"

#include "tit/py/cast.hpp"

using PyObject = struct _object; // NOLINT(*-reserved-identifier, cert-*)

namespace tit::py {

class Object;
class Tuple;
class Dict;
struct Kwarg;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Steal the reference to the object.
auto steal(PyObject* obj) -> Object;

/// Borrow the reference to the object.
auto borrow(PyObject* obj) -> Object;

/// A type that is not a Python object.
template<class Value>
concept not_object = !std::derived_from<std::remove_cvref_t<Value>, Object>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Base class for Python object references, encapsulates reference counting.
class BaseObject {
public:

  /// Construct a null object.
  BaseObject() = default;

  /// Construct an object by stealing the pointer.
  explicit BaseObject(PyObject* ptr);

  /// Move-construct an object.
  BaseObject(BaseObject&& other) noexcept;

  /// Copy-construct an object.
  BaseObject(const BaseObject& other) noexcept;

  /// Move-assign an object.
  auto operator=(BaseObject&& other) noexcept -> BaseObject&;

  /// Copy-assign an object.
  auto operator=(const BaseObject& other) -> BaseObject&;

  /// Destroy the object.
  ~BaseObject() noexcept;

  /// Check if the pointer is not null.
  auto valid() const noexcept -> bool;

  /// Get pointer to the object.
  auto get() const noexcept -> PyObject*;

  /// Release the pointer.
  auto release() noexcept -> PyObject*;

  /// Reset the pointer.
  void reset(PyObject* ptr);

  /// Increment the reference count.
  void incref() const noexcept;

  /// Decrement the reference count.
  void decref() const noexcept;

private:

  PyObject* ptr_ = nullptr;

}; // class BaseObject

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Python object reference.
class Object : public BaseObject {
public:

  using BaseObject::BaseObject;

  /// Reference wrapper to the sequence item or slice.
  template<class Self, class Index>
  class ItemAt {
  public:

    /// Item type.
    using Item = decltype(std::declval<Self&>().at(std::declval<Index>()));

    /// Construct a wrapper.
    constexpr ItemAt(const Self& self, Index index)
        : self_{&self}, index_{std::move(index)} {}

    /// Get the item or slice.
    template<std::constructible_from<Item> Value>
    constexpr explicit(false) operator Value() const {
      TIT_ASSERT(self_ != nullptr, "Self is invalid!");
      return Value{self_->at(index_)};
    }

    /// Assign the item or slice.
    template<class Value>
    auto operator=(Value&& value) -> ItemAt& {
      TIT_ASSERT(self_ != nullptr, "Self is invalid!");
      self_->set_at(index_, std::forward<Value>(value));
      return *this;
    }

  private:

    const Self* self_;
    Index index_;

  }; // class ItemAt

  /// Get the type name of the `Object`.
  static constexpr CStrView type_name = "object";

  /// Check if the object is a subclass of `Object`.
  static auto isinstance(const Object& obj) -> bool;

  /// Check if the object is another object.
  auto is(const Object& other) const -> bool;

  /// Check if the object has an attribute with the given name.
  /// @{
  auto has_attr(const Object& name) const -> bool;
  auto has_attr(CStrView name) const -> bool;
  /// @}

  /// Access the object attribute, similar to `obj.attr`.
  /// @{
  auto attr(const Object& name) const -> Object;
  auto attr(CStrView name) const -> Object;
  void set_attr(const Object& name, const Object& value) const;
  void set_attr(CStrView name, const Object& value) const;
  template<not_object Value>
  void set_attr(const Object& name, Value&& value) const {
    set_attr(name, object(std::forward<Value>(value)));
  }
  template<not_object Value>
  void set_attr(CStrView name, Value&& value) const {
    set_attr(name, object(std::forward<Value>(value)));
  }
  /// @}

  /// Delete the object attribute, similar to `del obj.attr`.
  /// @{
  void del_attr(const Object& name) const;
  void del_attr(CStrView name) const;
  /// @}

  /// Access the item with the given key, similar to `obj[key]`.
  /// @{
  auto at(const Object& key) const -> Object;
  void set_at(const Object& key, const Object& value) const;
  template<not_object Value>
  void set_at(const Object& key, Value&& value) const {
    set_at(key, object(std::forward<Value>(value)));
  }
  auto operator[](const Object& key) const -> ItemAt<Object, Object> {
    return {*this, key};
  }
  /// @}

  /// Delete the item with the given key, similar to `del obj[key]`.
  void del(const Object& key) const;

  /// Invoke the object.
  /// @{
  auto call() const -> Object;
  auto tp_call(const Tuple& posargs) const -> Object;
  auto tp_call(const Tuple& posargs, const Dict& kwargs) const -> Object;
  auto call(std::span<const Object> posargs) const -> Object;
  auto call(std::span<const Object> posargs,
            std::span<const Kwarg> kwargs) const -> Object;
  template<class... Args>
  auto operator()(Args&&... args) const -> Object;
  /// @}

  /// Check if the object represents a true value, similar to `bool(obj)`.
  explicit operator bool() const;

  /// Check if the object represents a false value, similar to `not obj`.
  auto operator!() const -> bool;

  /// Comparison operators.
  /// @{
  friend auto operator==(const Object& a, const Object& b) -> bool;
  friend auto operator!=(const Object& a, const Object& b) -> bool;
  friend auto operator<(const Object& a, const Object& b) -> bool;
  friend auto operator<=(const Object& a, const Object& b) -> bool;
  friend auto operator>(const Object& a, const Object& b) -> bool;
  friend auto operator>=(const Object& a, const Object& b) -> bool;
  /// @}

  /// Arithmetic operations.
  /// @{
  friend auto operator+(const Object& a) -> Object;
  friend auto operator-(const Object& a) -> Object;
  friend auto operator+(const Object& a, const Object& b) -> Object;
  friend auto operator-(const Object& a, const Object& b) -> Object;
  friend auto operator*(const Object& a, const Object& b) -> Object;
  friend auto operator/(const Object& a, const Object& b) -> Object;
  friend auto operator%(const Object& a, const Object& b) -> Object;
  friend auto operator+=(Object& a, const Object& b) -> Object&;
  friend auto operator-=(Object& a, const Object& b) -> Object&;
  friend auto operator*=(Object& a, const Object& b) -> Object&;
  friend auto operator/=(Object& a, const Object& b) -> Object&;
  friend auto operator%=(Object& a, const Object& b) -> Object&;
  /// @}

  /// Bitwise operations.
  /// @{
  friend auto operator~(const Object& a) -> Object;
  friend auto operator&(const Object& a, const Object& b) -> Object;
  friend auto operator|(const Object& a, const Object& b) -> Object;
  friend auto operator^(const Object& a, const Object& b) -> Object;
  friend auto operator<<(const Object& a, const Object& b) -> Object;
  friend auto operator>>(const Object& a, const Object& b) -> Object;
  friend auto operator&=(Object& a, const Object& b) -> Object&;
  friend auto operator|=(Object& a, const Object& b) -> Object&;
  friend auto operator^=(Object& a, const Object& b) -> Object&;
  friend auto operator<<=(Object& a, const Object& b) -> Object&;
  friend auto operator>>=(Object& a, const Object& b) -> Object&;
  /// @}

}; // class Object

/// Length of the object, similar to `len(obj)`.
auto len(const Object& obj) -> size_t;

/// Hash the object, similar to `hash(obj)`.
auto hash(const Object& obj) -> size_t;

/// String representation, similar to `str(obj)`.
auto str(const Object& obj) -> std::string;

/// Object representation, similar to `repr(obj)`.
auto repr(const Object& obj) -> std::string;

/// Absolute value of the object, similar to `abs(obj)`.
auto abs(const Object& obj) -> Object;

/// Floor division of the objects, similar to `a // b`.
/// @{
auto floordiv(const Object& a, const Object& b) -> Object;
auto floordiv_inplace(Object& a, const Object& b) -> Object&;
/// @}

/// Power of the objects, similar to `a ** b`.
/// @{
auto pow(const Object& a, const Object& b) -> Object;
auto pow_inplace(Object& a, const Object& b) -> Object&;
/// @}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// `None` literal.
auto None() -> Object;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Call keyword argument.
struct Kwarg final {
  CStrView name;
  Object value;
};

/// Check if the argument is a keyword argument.
template<class Arg>
concept is_kwarg = std::same_as<std::remove_cvref_t<Arg>, Kwarg>;

/// Make a keyword argument.
template<class Value>
auto kwarg(CStrView name, Value&& value) -> Kwarg {
  return Kwarg{name, object(std::forward<Value>(value))};
}

template<class... Args>
auto Object::operator()(Args&&... args) const -> Object {
  if constexpr (constexpr auto NumArgs = sizeof...(Args); NumArgs == 0) {
    // No arguments.
    return call();
  } else {
    if constexpr (constexpr auto NumKwargs = ((is_kwarg<Args> ? 1 : 0) + ...);
                  NumKwargs == 0) {
      // Only positional arguments.
      return call(std::array{object(std::forward<Args>(args))...});
    } else {
      // Positional and keyword arguments.
      constexpr auto NumPosArgs = NumArgs - NumKwargs;
      std::array<Object, NumPosArgs> posargs;
      std::array<Kwarg, NumKwargs> kwargs;
      auto posarg_iter = posargs.begin();
      auto kwarg_iter = kwargs.begin();
      const auto fill_arg = [&posarg_iter, &kwarg_iter]<class Arg>(Arg&& arg) {
        if constexpr (is_kwarg<Arg>) {
          *kwarg_iter = std::forward<Arg>(arg);
          ++kwarg_iter;
        } else {
          *posarg_iter = object(std::forward<Arg>(arg));
          ++posarg_iter;
        }
      };
      (fill_arg(std::forward<Args>(args)), ...);
      return call(posargs, kwargs);
    }
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::py
