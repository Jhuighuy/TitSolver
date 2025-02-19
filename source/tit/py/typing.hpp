/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <concepts>
#include <string>
#include <utility>

#include "tit/core/str_utils.hpp"
#include "tit/core/type_utils.hpp"

#include "tit/py/object.hpp"

namespace tit::py {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Any object.
using Any = Object;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Object that can be one of several types.
template<std::derived_from<Object>... Options>
  requires (sizeof...(Options) > 1) && all_unique_v<Options...>
class Union final : public Object {
public:

  // Nested unions are not allowed.
  static_assert((!specialization_of<Options, Union> && ...));

  /// Get the type name of the `Union`.
  static auto type_name() -> std::string {
    return join<" | ">(py::type_name<Options>()...);
  }

  /// Check if the object is a subclass of `Union`.
  static auto isinstance(const Object& obj) -> bool {
    return (Options::isinstance(obj) || ...);
  }

  /// Construct a variant object.
  template<std::derived_from<Object> Option>
    requires contains_v<Option, Options...>
  explicit(false) Union(Option obj) : Object{std::move(obj)} {}

}; // class Union

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Optional object reference.
template<std::derived_from<Object> Option>
using Optional = Union<Option, NoneType>;

/// Maybe steal the reference to the object if it is not null.
template<std::derived_from<Object> Derived = Object>
auto maybe_steal(PyObject* obj) -> Optional<Derived> {
  if (obj != nullptr) return steal<Derived>(obj);
  return None();
}

/// Maybe borrow the reference to the object if it is not null.
template<std::derived_from<Object> Derived = Object>
auto maybe_borrow(PyObject* obj) -> Optional<Derived> {
  if (obj != nullptr) return borrow<Derived>(obj);
  return None();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::py
