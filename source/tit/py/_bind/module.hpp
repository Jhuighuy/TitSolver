/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// IWYU pragma: private, include "tit/py/bind.hpp"
#pragma once

#include "tit/core/str_utils.hpp"

#include "tit/py/_bind/func.hpp" // IWYU pragma: export
#include "tit/py/core.hpp"

namespace tit::py::cpp {

// NOLINTBEGIN(*-include-cleaner)

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Reference to a Python module.
class Module final : public py::Module {
public:

  /// Add a new module object.
  void add(CStrView name, const Object& obj) const {
    ensure(PyModule_AddObjectRef(get(), name.c_str(), obj.get()));
  }

  /// Define a new function in the module.
  template<StrLiteral Name, auto Func, param_spec... Params>
    requires func_spec<Func, Params...>
  void def() const {
    static auto def = make_func_def<Name, Func, Params...>();
    const auto func = steal<Object>(
        PyCFunction_NewEx(&def, /*self=*/nullptr, /*module=*/get()));
    add(Name.c_str(), func);
  }

}; // class Module

/// Import the module by name, similar to `import abc`.
inline auto import_(CStrView name) -> Module {
  return steal<Module>(PyImport_ImportModule(name.c_str()));
}

/// Create a new module.
template<StrLiteral Name>
auto module_() -> Module {
  static PyModuleDef module_def{
      .m_base = PyModuleDef_HEAD_INIT,
      .m_name = Name.c_str(),
      .m_doc = nullptr,
      .m_size = -1,
      .m_methods = nullptr,
      .m_slots = nullptr,
      .m_traverse = nullptr,
      .m_clear = nullptr,
      .m_free = nullptr,
  };
  return steal<Module>(PyModule_Create(&module_def));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Define a new Python module.
#define TIT_PYCPP_MODULE(name, func)                                           \
  /* NOLINTBEGIN(*-use-trailing-return-type) */                                \
  PyMODINIT_FUNC PyInit_##name();                                              \
  PyMODINIT_FUNC PyInit_##name() {                                             \
    auto m = tit::py::cpp::module_<#name>();                                   \
    func(m);                                                                   \
    return m.release();                                                        \
  }                                                                            \
  /* NOLINTEND(*-use-trailing-return-type) */

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// NOLINTEND(*-include-cleaner)

} // namespace tit::py::cpp
