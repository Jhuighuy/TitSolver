/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm>
#include <array>
#include <bit>
#include <concepts>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <typeinfo>
#include <utility>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/str_utils.hpp"
#include "tit/core/uint_utils.hpp"

using PyObject = struct _object; // NOLINT(*-reserved-identifier, cert-*)

namespace tit::py {

class Object;
class Tuple;
class Dict;
class Type;
struct Kwarg;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Steal the reference to the object.
auto steal(PyObject* ptr) -> Object;

/// Borrow the reference to the object.
auto borrow(PyObject* ptr) -> Object;

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
  class ItemAt final {
  public:

    /// Item type.
    using Item = decltype(std::declval<Self&>().at(std::declval<Index>()));

    /// Construct a wrapper.
    constexpr ItemAt(const Self& self, Index index)
        : self_{&self}, index_{std::move(index)} {}

    /// Reference wrapper is copyable and movable, but not assignable.
    /// @{
    constexpr ItemAt(ItemAt&&) noexcept = default;
    constexpr ItemAt(const ItemAt&) noexcept = default;
    auto operator=(ItemAt&& other) -> ItemAt& = delete;
    auto operator=(const ItemAt& other) -> ItemAt& = delete;
    constexpr ~ItemAt() noexcept = default;
    /// @}

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
  static consteval auto type_name() -> CStrView {
    return "object";
  }

  /// Construct an object from the given value.
  /// @{
  explicit Object(bool value);
  explicit Object(long long value);
  explicit Object(double value);
  explicit Object(std::string_view value);
  template<not_object Value>
  explicit Object(Value&& value);
  /// @}

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
    set_attr(name, Object{std::forward<Value>(value)});
  }
  template<not_object Value>
  void set_attr(CStrView name, Value&& value) const {
    set_attr(name, Object{std::forward<Value>(value)});
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
    set_at(key, Object{std::forward<Value>(value)});
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

/// None object reference.
class NoneType final : public Object {
public:

  /// Get the type object of the `NoneType`.
  static consteval auto type_name() -> CStrView {
    return "NoneType";
  }

  /// Check if the object is a subclass of `NoneType`.
  static auto isinstance(const Object& obj) -> bool;

}; // class NoneType

/// `None` literal.
auto None() -> NoneType;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Get the parent object of the instance.
template<class T>
struct ObjectParent final {
  auto operator()(const T& self) const -> Object = delete;
};

namespace impl {

// `PyObject` structure size / alignment in bytes.
extern const size_t sizeof_PyObject;
extern const size_t alignof_PyObject;

// Find the already bound type or raise an error. Defined in `type.cpp`.
auto lookup_type(const std::type_info& type_info) -> const Type&;

// Allocate a new uninitialized object of the given bound type / free it.
auto alloc(const std::type_info& type_info) -> PyObject*;
void dealloc(PyObject* ptr);

// Bound object structure alignment (in bytes).
template<class T>
auto alignof_instance() noexcept -> size_t {
  return std::max(alignof_PyObject, alignof(T));
}

// Offset of the data in the bound object structure (in bytes).
template<class T>
auto offsetof_data() noexcept -> size_t {
  return align_up(sizeof_PyObject, alignof_instance<T>());
}

// Size of the bound object structure (in bytes).
template<class T>
auto sizeof_instance() noexcept -> size_t {
  return offsetof_data<T>() + align_up(sizeof(T), alignof_instance<T>());
}

// Get the pointer to the instance data.
template<class T>
auto data(PyObject* ptr) noexcept -> T* {
  lookup_type(typeid(T));
  TIT_ASSERT(ptr != nullptr, "Object must not be null!");
  return std::bit_cast<T*>(std::bit_cast<byte_t*>(ptr) + offsetof_data<T>());
}

// Is the parent object specified?
template<class T>
concept has_parent = requires (ObjectParent<T> getter, T& self) {
  { getter(self) } -> std::derived_from<Object>;
};

// Initialize the instance data from the given arguments.
template<class T, class... Args>
  requires std::constructible_from<T, Args&&...>
void init_(T* self, Args&&... args) {
  lookup_type(typeid(T));
  TIT_ASSERT(self != nullptr, "Pointer must not be null!");
  std::construct_at(self, std::forward<Args>(args)...);
  if constexpr (const ObjectParent<T> parent_getter{}; has_parent<T>) {
    parent_getter(*self).incref();
  }
}

// Deinitialize the instance data and delete the object.
template<class T>
void delete_(PyObject* ptr) {
  T* const self = data<T>(ptr);
  [[maybe_unused]] Object parent;
  if constexpr (const ObjectParent<T> parent_getter{}; has_parent<T>) {
    parent = &parent_getter(*self);
  }
  std::destroy_at(self);
  dealloc(ptr);
  if constexpr (has_parent<T>) parent.decref();
}

} // namespace impl

/// Find a Python object holding the given bound class instance.
template<class T>
  requires std::is_class_v<T>
auto find(const T& self) -> Object {
  impl::lookup_type(typeid(T));
  return borrow(std::bit_cast<PyObject*>(
      std::bit_cast<byte_t*>(std::addressof(self)) - impl::offsetof_data<T>()));
}

/// Create a new bound type instance object.
template<class T, class... Args>
  requires std::constructible_from<T, Args&&...>
auto new_(Args&&... args) -> Object {
  const auto self = steal(impl::alloc(typeid(T)));
  impl::init_(impl::data<T>(self.get()), std::forward<Args>(args)...);
  return self;
}

template<not_object Value>
Object::Object(Value&& value) {
  using T = std::remove_cvref_t<Value>;
  if constexpr (std::integral<T>) {
    *this = Object{static_cast<long long>(value)};
  } else if constexpr (std::floating_point<T>) {
    *this = Object{static_cast<double>(value)};
  } else if constexpr (str_like<T>) {
    *this = Object{std::string_view{value}};
  } else if constexpr (std::convertible_to<T, Object>) {
    *this = std::forward<Value>(value);
  } else {
    *this = new_<T>(std::forward<Value>(value));
  }
}

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
  return Kwarg{.name = name, .value = Object{std::forward<Value>(value)}};
}

template<class... Args>
auto Object::operator()(Args&&... args) const -> Object {
  if constexpr (constexpr auto NumArgs = sizeof...(Args); NumArgs == 0) {
    // No arguments.
    return call();
  } else if constexpr (constexpr auto NumKwargs =
                           (size_t{is_kwarg<Args>} + ...);
                       NumKwargs == 0) {
    // Only positional arguments.
    return call(std::array{Object{std::forward<Args>(args)}...});
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
        *posarg_iter = Object{std::forward<Arg>(arg)};
        ++posarg_iter;
      }
    };
    (fill_arg(std::forward<Args>(args)), ...);
    TIT_ASSERT(posarg_iter == posargs.end(), "Error parsing arguments!");
    TIT_ASSERT(kwarg_iter == kwargs.end(), "Error parsing arguments!");
    return call(posargs, kwargs);
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::py
