/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <concepts>
#include <string>
#include <string_view>
#include <typeinfo>
#include <unordered_map>

#include "tit/core/basic_types.hpp"

#include "tit/py/object.hpp"

// NOLINTNEXTLINE(*-reserved-identifier, cert-*)
using PyTypeObject = struct _typeobject;

namespace tit::py {

class Module;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Python type reference.
class Type : public Object {
public:

  /// Get the type object of the `Type`.
  static auto type() -> Type;

  /// Check if the object is a subclass of `Type`.
  static auto isinstance(const Object& obj) -> bool;

  /// Get pointer to the object as `PyTypeObject*`.
  auto get_type() const -> PyTypeObject*;

  /// Get the name of the type.
  auto name() const -> std::string;

  /// Get the qualified name of the type.
  auto qualified_name() const -> std::string;

  /// Get the fully qualified name of the type.
  auto fully_qualified_name() const -> std::string;

  /// Get the name of the module which defines the type.
  auto module_name() const -> std::string;

  /// Check if this type a inherited from a different type.
  auto is_subtype_of(const Type& other) const -> bool;

protected:

  /// Construct a new reference to the existing type object.
  explicit Type(PyObject* ptr);

}; // class Type

/// Get the type of the given object, similar to `type(obj)`.
auto type(const Object& obj) -> Type;

/// Borrow the type object pointer.
auto borrow(PyTypeObject* type) -> Type;

/// Get the type name of the given C++ type.
template<std::derived_from<Object> Derived>
auto type_name() -> std::string {
  if constexpr (requires { Derived::type(); }) {
    return Derived::type().fully_qualified_name();
  } else if constexpr (requires { Derived::type_name(); }) {
    return std::string{Derived::type_name()};
  } else static_assert(false);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Python destructor function pointer.
using DestructorPtr = void (*)(PyObject*);

/// Python heap type reference.
class HeapType : public Type {
public:

  /// Find the type object by its type info.
  static auto find(const std::type_info& type_info) -> const HeapType&;

  /// Construct a new heap type.
  HeapType(const std::type_info& type_info,
           std::string_view name,
           size_t basic_size,
           DestructorPtr destructor,
           const Module& module_);

private:

  // Map of bound types.
  static std::unordered_map<size_t, HeapType> types_;

}; // class HeapType

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::py
