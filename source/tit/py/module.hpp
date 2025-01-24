/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include "tit/core/str_utils.hpp"

#include "tit/py/mapping.hpp"
#include "tit/py/object.hpp"
#include "tit/py/type.hpp"

namespace tit::py {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Reference to a Python module.
class Module final : public Object {
public:

  /// Get the type object of the `Bool`.
  static auto type() -> Type;

  /// Check if the object is a subclass of `Module`.
  static auto isinstance(const Object& obj) -> bool;

  /// Get the module name.
  auto name() const -> CStrView;

  /// Get the module dictionary.
  auto dict() const -> Dict;

}; // class Module

/// Import the module by name, similar to `import name`.
auto import_(CStrView name) -> Module;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::py
