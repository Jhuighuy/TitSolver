/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <bit>
#include <forward_list>

#include "tit/core/checks.hpp"

#include "tit/py/_python.hpp"
#include "tit/py/func.hpp"
#include "tit/py/module.hpp"
#include "tit/py/sequence.hpp"

namespace tit::py::impl {

// NOLINTBEGIN(*-include-cleaner)

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

namespace {

// Container for function definitions.
class FuncDefs final {
public:

  // Emplace a function definition.
  auto emplace(const char* name, FuncPtr func) -> PyMethodDef& {
    TIT_ASSERT(name != nullptr, "Function name must not be null!");
    TIT_ASSERT(func != nullptr, "Function pointer must not be null!");
    return defs_.emplace_front(PyMethodDef{
        .ml_name = name,
        .ml_meth = std::bit_cast<PyCFunction>(func),
        .ml_flags = METH_VARARGS | METH_KEYWORDS,
        .ml_doc = nullptr,
    });
  }

private:

  // Pointers to the function definitions must be kept alive until the
  // extension is unloaded, so we need to keep them in a node-based container.
  std::forward_list<PyMethodDef> defs_;

}; // class FuncDefs

// Container for property definitions.
class PropDefs final {
public:

  // Emplace a property definition.
  auto emplace(const char* name, GetPtr get, SetPtr set) -> PyGetSetDef& {
    TIT_ASSERT(name != nullptr, "Property name must not be null!");
    TIT_ASSERT(get != nullptr, "Property getter pointer must not be null!");
    return defs_.emplace_front(PyGetSetDef{
        .name = name,
        .get = get,
        .set = set,
        .doc = nullptr,
        .closure = nullptr,
    });
  }

private:

  // Pointers to the property definitions must be kept alive until the
  // extension is unloaded, so we need to keep them in a node-based container.
  std::forward_list<PyGetSetDef> defs_;

}; // class PropDefs

FuncDefs func_defs;
PropDefs prop_defs;

} // namespace

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto count_args(PyObject* posargs, PyObject* kwargs) -> size_t {
  TIT_ASSERT(posargs != nullptr, "Positional arguments must not be null!");
  auto result = len(borrow<Tuple>(posargs));
  if (kwargs != nullptr) result += len(borrow<Dict>(kwargs));
  return result;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto make_func(const char* name, FuncPtr func, const Module* module_)
    -> Object {
  return steal(ensure( //
      PyCFunction_NewEx(&func_defs.emplace(name, func),
                        /*self=*/nullptr,
                        module_ != nullptr ? module_->get() : nullptr)));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto make_method_descriptor(const char* name,
                            FuncPtr method,
                            const Type& class_) -> Object {
  return steal(ensure(
      PyDescr_NewMethod(class_.get_type(), &func_defs.emplace(name, method))));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto make_prop_descriptor(const char* name,
                          GetPtr get,
                          SetPtr set,
                          const Type& class_) -> Object {
  return steal(ensure( //
      PyDescr_NewGetSet(class_.get_type(),
                        &prop_defs.emplace(name, get, set))));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// NOLINTEND(*-include-cleaner)

} // namespace tit::py::impl
