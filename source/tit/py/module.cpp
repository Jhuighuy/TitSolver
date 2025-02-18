/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <forward_list>

#include "tit/core/checks.hpp"
#include "tit/core/str_utils.hpp"

#include "tit/py/_python.hpp"
#include "tit/py/error.hpp"
#include "tit/py/mapping.hpp"
#include "tit/py/module.hpp"
#include "tit/py/object.hpp"

namespace tit::py {

// NOLINTBEGIN(*-include-cleaner)

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

namespace {

// Container for module definitions.
class ModuleDefs final {
public:

  // Emplace a module definition.
  auto emplace(const char* name) -> PyModuleDef& {
    TIT_ASSERT(name != nullptr, "Module name must not be null!");
    return defs_.emplace_front(PyModuleDef{
        .m_base = PyModuleDef_HEAD_INIT,
        .m_name = name,
        .m_doc = nullptr,
        .m_size = -1, // holds global state.
        .m_methods = nullptr,
        .m_slots = nullptr,
        .m_traverse = nullptr,
        .m_clear = nullptr,
        .m_free = nullptr,
    });
  }

private:

  // Pointers to the module definitions must be kept alive until the extension
  // is unloaded, so we need to keep them in a node-based container.
  std::forward_list<PyModuleDef> defs_;

}; // class ModuleDefs

ModuleDefs module_defs;

} // namespace

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto Module::type() -> Type {
  return borrow(&PyModule_Type);
}

auto Module::isinstance(const Object& obj) -> bool {
  return ensure(PyModule_Check(obj.get()));
}

Module::Module(const char* name)
    : Object{ensure(PyModule_Create(&module_defs.emplace(name)))} {}

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
void Module::add(const Type& type) const {
  add(type.name(), type);
}

auto import_(CStrView name) -> Module {
  return steal<Module>(ensure(PyImport_ImportModule(name.c_str())));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// NOLINTEND(*-include-cleaner)

} // namespace tit::py
