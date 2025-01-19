/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// IWYU pragma: private, include "tit/py/core.hpp"
#pragma once

#include "tit/core/str_utils.hpp"

#include "tit/py/_core/_python.hpp"
#include "tit/py/_core/objects.hpp"

namespace tit::py {

// NOLINTBEGIN(*-include-cleaner)

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Reference to a Python module.
class Module : public Object {
public:

  /// Check if the object is a subclass of `Module`.
  static auto isinstance(const Object& obj) -> bool {
    return ensure(PyModule_Check(obj.get()));
  }

  /// Get the module name.
  auto name() const -> CStrView {
    return CStrView{PyModule_GetName(get())};
  }

  /// Get the module dictionary.
  auto dict() const -> Dict {
    return borrow<Dict>(PyModule_GetDict(get()));
  }

}; // class Module

/// Import the module by name, similar to `import name`.
inline auto import_(CStrView name) -> Module {
  return steal<Module>(PyImport_ImportModule(name.c_str()));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// NOLINTEND(*-include-cleaner)

} // namespace tit::py
