/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <bit>
#include <forward_list>
#include <string>
#include <utility>

#include "tit/core/checks.hpp"

#include "tit/py/_python.hpp"
#include "tit/py/func.hpp"
#include "tit/py/module.hpp"
#include "tit/py/type.hpp"

namespace tit::py {

// NOLINTBEGIN(*-include-cleaner)

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

namespace {

// Make a function definition.
auto make_func_def(std::string name, CppFuncPtr func) -> PyMethodDef& {
  TIT_ASSERT(!name.empty(), "Function name must not be empty!");
  TIT_ASSERT(func != nullptr, "Function pointer must not be null!");
  static std::forward_list<std::string> names;
  static std::forward_list<PyMethodDef> defs;
  return defs.emplace_front(PyMethodDef{
      .ml_name = names.emplace_front(std::move(name)).c_str(),
      .ml_meth = std::bit_cast<PyCFunction>(func),
      .ml_flags = METH_VARARGS | METH_KEYWORDS,
      .ml_doc = nullptr,
  });
}

} // namespace

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto CFunction::type() -> Type {
  return borrow(&PyCFunction_Type);
}

auto CFunction::isinstance(const Object& obj) -> bool {
  return ensure(PyCFunction_Check(obj.get()));
}

CFunction::CFunction(std::string name, CppFuncPtr func, const Module* module_)
    : Object{ensure(PyCFunction_NewEx( //
          &make_func_def(std::move(name), func),
          /*self=*/nullptr,
          module_ != nullptr ? module_->get() : nullptr))} {}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// NOLINTEND(*-include-cleaner)

} // namespace tit::py
