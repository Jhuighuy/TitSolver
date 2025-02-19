/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include "tit/core/str_utils.hpp"

#include "tit/py/func.hpp"
#include "tit/py/mapping.hpp"
#include "tit/py/object.hpp"

namespace tit::py {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Reference to a Python module.
class Module final : public Object {
public:

  /// Get the type object of the `Bool`.
  static auto type() -> Type;

  /// Check if the object is a subclass of `Module`.
  static auto isinstance(const Object& obj) -> bool;

  /// Create a new module.
  explicit Module(const char* name);

  /// Get the module name.
  auto name() const -> CStrView;

  /// Get the module dictionary.
  auto dict() const -> Dict;

  /// Define a new module object.
  /// @{
  void add(CStrView name, const Object& obj) const;
  template<not_object Value>
  void add(CStrView name, Value&& value) const {
    add(name, object(std::forward<Value>(value)));
  }
  void add(const Type& type) const;
  /// @}

  /// Define a new module function.
  template<StrLiteral Name, auto Func, param_spec... Params>
    requires func_spec<Func, Params...>
  void def() const {
    add(Name, make_func<Name, Func, Params...>(this));
  }

}; // class Module

/// Import the module by name, similar to `import name`.
auto import_(CStrView name) -> Module;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Define a new Python module.
#define TIT_PYTHON_MODULE(name, func)                                          \
  extern "C" [[gnu::visibility("default")]] auto PyInit_##name()->PyObject*;   \
  auto PyInit_##name()->PyObject* {                                            \
    using namespace tit;                                                       \
    return py::impl::translate_exceptions<nullptr>([] {                        \
      py::Module module_{#name};                                               \
      func(module_);                                                           \
      return module_.release();                                                \
    });                                                                        \
  }

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::py
