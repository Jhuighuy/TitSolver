/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <bit>

#include "tit/py/_python.hpp"
#include "tit/py/cast.hpp"
#include "tit/py/error.hpp"
#include "tit/py/object.hpp"
#include "tit/py/type.hpp"

namespace tit::py {

// NOLINTBEGIN(*-include-cleaner)

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto Type::type() -> Type {
  return borrow(&PyType_Type);
}

auto Type::isinstance(const Object& obj) -> bool {
  return ensure(PyType_Check(obj.get()));
}

auto Type::get_type() const -> PyTypeObject* {
  return std::bit_cast<PyTypeObject*>(get());
}

/// @todo Use `PyType_GetName` once we have Python 3.11.
auto Type::name() const -> std::string {
  return extract<std::string>(attr("__name__"));
}

/// @todo Use `PyType_GetQualName` once we have Python 3.11.
auto Type::qualified_name() const -> std::string {
  return extract<std::string>(attr("__qualname__"));
}

/// @todo Use `PyType_GetFullyQualifiedName` once we have Python 3.13.
auto Type::fully_qualified_name() const -> std::string {
  const auto mod_name = module_name();
  const auto qual_name = qualified_name();
  return mod_name == "builtins" ? qual_name :
                                  std::format("{}.{}", mod_name, qual_name);
}

/// @todo Use `PyType_GetModuleName` once we have Python 3.13.
auto Type::module_name() const -> std::string {
  return extract<std::string>(attr("__module__"));
}

auto Type::is_subtype_of(const Type& other) const -> bool {
  return ensure(PyType_IsSubtype(get_type(), other.get_type()));
}

auto type(const Object& obj) -> Type {
  return steal<Type>(ensure(PyObject_Type(obj.get())));
}

auto borrow(PyTypeObject* type) -> Type {
  return borrow<Type>(std::bit_cast<PyObject*>(type));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// NOLINTEND(*-include-cleaner)

} // namespace tit::py
