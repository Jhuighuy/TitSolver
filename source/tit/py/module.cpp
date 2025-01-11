/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <forward_list>
#include <string>
#include <utility>

#include "tit/core/checks.hpp"
#include "tit/core/str_utils.hpp"

#include "tit/py/_python.hpp"
#include "tit/py/error.hpp"
#include "tit/py/mapping.hpp"
#include "tit/py/module.hpp"
#include "tit/py/object.hpp"
#include "tit/py/type.hpp"

namespace tit::py {

// NOLINTBEGIN(*-include-cleaner)

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

namespace {

// Make a module definition.
auto make_module_def(std::string name) -> PyModuleDef& {
  TIT_ASSERT(!name.empty(), "Module name must not be empty!");
  static std::forward_list<std::string> names;
  static std::forward_list<PyModuleDef> defs;
  return defs.emplace_front(PyModuleDef{
      .m_base = PyModuleDef_HEAD_INIT,
      .m_name = names.emplace_front(std::move(name)).c_str(),
      .m_doc = nullptr,
      .m_size = -1,
      .m_methods = nullptr,
      .m_slots = nullptr,
      .m_traverse = nullptr,
      .m_clear = nullptr,
      .m_free = nullptr,
  });
}

} // namespace

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto Module::type() -> Type {
  return borrow(&PyModule_Type);
}

auto Module::isinstance(const Object& obj) -> bool {
  return ensure(PyModule_Check(obj.get()));
}

auto Module::name() const -> CStrView {
  const auto* const result = PyModule_GetName(get());
  ensure_no_error();
  TIT_ASSERT(result != nullptr, "String is null, but no error occurred!");
  return CStrView{result};
}

auto Module::dict() const -> Dict {
  return borrow<Dict>(ensure(PyModule_GetDict(get())));
}

void Module::add(CStrView name, const Object& obj) const {
  ensure(PyModule_AddObjectRef(get(), name.c_str(), obj.get()));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto import_(CStrView name) -> Module {
  return steal<Module>(ensure(PyImport_ImportModule(name.c_str())));
}

auto module_(std::string name) -> Module {
  return steal<Module>(PyModule_Create(&make_module_def(std::move(name))));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// NOLINTEND(*-include-cleaner)

} // namespace tit::py
