/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

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

auto import_(CStrView name) -> Module {
  return steal<Module>(ensure(PyImport_ImportModule(name.c_str())));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// NOLINTEND(*-include-cleaner)

} // namespace tit::py
