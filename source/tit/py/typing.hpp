/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <concepts>
#include <string>
#include <utility>

#include "tit/core/str_utils.hpp"
#include "tit/core/type_utils.hpp"

#include "tit/py/cast.hpp"
#include "tit/py/object.hpp"
#include "tit/py/type.hpp"

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

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::py
