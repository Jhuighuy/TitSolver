/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <concepts>
#include <memory>
#include <string_view>
#include <typeinfo>

#include "tit/core/basic_types.hpp"
#include "tit/core/str_utils.hpp"

#include "tit/py/error.hpp"
#include "tit/py/func.hpp"
#include "tit/py/module.hpp"
#include "tit/py/object.hpp"

namespace tit::py {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

namespace impl {

// Python destructor function pointer.
using DestructorPtr = void (*)(PyObject*);

// Bind a new type.
auto bind_class(const std::type_info& type_info,
                std::string_view name,
                size_t basic_size,
                DestructorPtr destructor,
                const Module& module_) -> Type;

} // namespace impl

/// Python type reference of a bound type.
template<class T>
class Class final : public Type {
public:

  /// Bind a new class.
  Class(CStrView name, const Module& module_)
      : Type{impl::bind_class(typeid(T),
                              name,
                              impl::sizeof_instance<T>(),
                              &impl::delete_<T>,
                              module_)} {
    def<"__init__", [](T& self) {
      raise_type_error("cannot create '{}' instances",
                       py::type(find(self)).fully_qualified_name());
    }>();
  }

  /// Define a new property in the class.
  template<StrLiteral Name, auto Get, auto Set = nullptr>
    requires prop_spec<T, Get, Set>
  void prop() const {
    set_attr(Name, make_prop_descriptor<Name, T, Get, Set>(*this));
  }

  /// Define a new method in the class.
  template<StrLiteral Name, auto Method, param_spec... Params>
    requires method_spec<T, Method, Params...>
  void def() const {
    set_attr(Name, make_method_descriptor<Name, T, Method, Params...>(*this));
  }

  /// Define a `__init__` method in the class.
  template<param_spec... Params>
    requires std::constructible_from<T, typename Params::type...>
  void def_init() const {
    def<"__init__",
        [](T& self, typename Params::type... args) {
          impl::init_data(std::addressof(self), std::move(args)...);
        },
        Params...>();
  }

}; // class Class

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::py
