/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <concepts>
#include <string>

#include "tit/py/object.hpp"

// NOLINTNEXTLINE(*-reserved-identifier, cert-*)
using PyTypeObject = struct _typeobject;

namespace tit::py {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Python type reference.
class Type : public Object {
public:

  /// Get the type object of the `Type`.
  static auto type() -> Type;

  /// Check if the object is a subclass of `Type`.
  static auto isinstance(const Object& obj) -> bool;

  /// Get pointer to the object.
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
  } else if constexpr (requires { Derived::type_name; }) {
    /// @todo In C++26 there would be no need for `std::string{...}`.
    return std::string{Derived::type_name};
  } else static_assert(false);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::py
