/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm>
#include <array>
#include <concepts>
#include <cstddef>
#include <exception>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <utility>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/str_utils.hpp"

#include "tit/py/error.hpp"
#include "tit/py/mapping.hpp"
#include "tit/py/object.hpp"
#include "tit/py/sequence.hpp"

namespace tit::py {

class Module;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Argument factory.
template<class Type>
using Factory = Type (*)();

/// Default argument specification: either a `nullptr`, a factory function,
/// or a compatible value.
template<class Default, class Type>
concept default_spec =
    std::convertible_to<Default, Factory<Type>> ||
    std::same_as<Default, Type> || std::same_as<Default, std::nullptr_t>;

namespace impl {

// Convert the default argument to a function.
template<class Type, default_spec<Type> auto Default>
consteval auto make_factory() -> Factory<Type> {
  using D = decltype(Default);
  if constexpr (std::convertible_to<D, Factory<Type>>) return Default;
  else if constexpr (std::same_as<D, Type>) return [] { return Default; };
  else if constexpr (std::same_as<D, std::nullptr_t>) return nullptr;
  else static_assert(false);
}

} // namespace impl

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Function parameter specification.
template<class Type, StrLiteral Name, Factory<Type> Default>
struct ParamSpec final {
  using type = Type;                        ///< Parameter type.
  static constexpr CStrView name = Name;    ///< Parameter name.
  static constexpr auto default_ = Default; ///< Default value factory.
};

/// Function parameter specification.
template<class Type, StrLiteral Name, default_spec<Type> auto Default = nullptr>
using Param = ParamSpec<Type, Name, impl::make_factory<Type, Default>()>;

namespace impl {

template<class>
struct is_param : std::false_type {};

template<class Type, StrLiteral Name, Factory<Type> Default>
struct is_param<ParamSpec<Type, Name, Default>> : std::true_type {};

} // namespace impl

/// Function parameter specification type.
template<class Param>
concept param_spec = impl::is_param<Param>::value;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

namespace impl {

// Count the number of arguments.
auto count_args(PyObject* posargs, PyObject* kwargs) -> size_t;

// Parse a single function argument.
template<param_spec Param>
auto parse_single_arg(const Object& arg) -> typename Param::type {
  // Fill the default argument value.
  if (!arg.valid()) {
    if constexpr (Param::default_ == nullptr) {
      raise_type_error("missing argument '{}'", Param::name);
    }
    return Param::default_();
  }

  // Extract the argument value.
  try {
    return extract<typename Param::type>(arg);
  } catch (ErrorException& e) {
    e.prefix_message("argument '{}'", Param::name);
    throw;
  }
}

// Parse all the function arguments.
template<param_spec... Params>
auto parse_args(PyObject* posargs, PyObject* kwargs)
    -> std::tuple<typename Params::type...> {
  static constexpr auto num_params = sizeof...(Params);
  std::array<Object, num_params> args;

  // Unpack positional arguments.
  TIT_ASSERT(posargs != nullptr, "Positional arguments must not be null!");
  const auto posargs_ = borrow<Tuple>(posargs);
  const auto num_posargs = len(posargs_);
  if (num_posargs > num_params) {
    raise_type_error("function takes at most {} arguments ({} given)",
                     num_params,
                     count_args(posargs, kwargs));
  }
  for (size_t i = 0; i < num_posargs; ++i) args[i] = posargs_[i];

  // Unpack keyword arguments.
  if (kwargs != nullptr) {
    static constexpr std::array param_names{Params::name...};
    const auto kwargs_ = borrow<Dict>(kwargs);
    kwargs_.for_each([&args](const Object& arg_name, const Object& arg) {
      const auto arg_name_str = extract<CStrView>(arg_name);
      const auto param_iter = std::ranges::find(param_names, arg_name_str);
      if (param_iter == param_names.end()) {
        raise_type_error("unexpected argument '{}'", arg_name_str);
      }
      const auto param_index = std::distance(param_names.begin(), param_iter);
      if (args[param_index].valid()) {
        raise_type_error("duplicate argument '{}'", arg_name_str);
      }
      args[param_index] = arg;
    });
  }

  // Parse the arguments.
  return std::apply(
      [](auto... args_) {
        return std::tuple{parse_single_arg<Params>(std::move(args_))...};
      },
      std::move(args));
}
template<>
inline auto parse_args(PyObject* posargs, PyObject* kwargs) -> std::tuple<> {
  if (const auto num_args = count_args(posargs, kwargs); num_args > 0) {
    raise_type_error("function takes no arguments ({} given)", num_args);
  }
  return {};
}

// Invoke the function, and return the result as a Python object.
template<StrLiteral Name, param_spec... Params, class Func>
auto invoke(Func func, PyObject* posargs, PyObject* kwargs) -> Object {
  // Parse the arguments.
  auto args = [posargs, kwargs] {
    try {
      return impl::parse_args<Params...>(posargs, kwargs);
    } catch (ErrorException& e) {
      e.prefix_message("function '{}'", Name);
      throw;
    }
  }();

  // Apply the arguments to the function and wrap the result into an object.
  const auto apply_args = [&func, args] {
    return std::apply(func, std::move(args));
  };
  if constexpr (std::same_as<decltype(apply_args()), void>) {
    apply_args();
    return None();
  } else {
    return object(apply_args());
  }
}

// Call the function, and return the result or set the error.
template<auto OnError, std::invocable Func>
  requires std::convertible_to<decltype(OnError), std::invoke_result_t<Func>>
auto translate_exceptions(Func func) noexcept -> std::invoke_result_t<Func> {
  try {
    return func();
  } catch (ErrorException& e) {
    e.restore();
  } catch (const std::logic_error& e) {
    set_assertion_error(e.what());
  } catch (const std::exception& e) {
    set_system_error(e.what());
  } catch (...) {
    set_system_error("unknown error.");
  }
  TIT_ASSERT(is_error_set(), "Python error must be set!");
  return OnError;
}

} // namespace impl

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

namespace impl {

// Python function pointer.
using FuncPtr = PyObject* (*) (PyObject*, PyObject*, PyObject*);

// Construct a new function object from the given function pointer.
auto make_func(const char* name, FuncPtr func, const Module* module_) -> Object;

} // namespace impl

/// Function specification.
template<auto Func, class... Params>
concept func_spec = (param_spec<Params> && ...) &&
                    (std::invocable<decltype(Func), typename Params::type...>);

/// Construct a new function object from the given specification.
template<StrLiteral Name, auto Func, param_spec... Params>
  requires func_spec<Func, Params...>
auto make_func(const Module* module_ = nullptr) -> Object {
  constexpr impl::FuncPtr func_wrapper =
      [](PyObject* self, PyObject* posargs, PyObject* kwargs) -> PyObject* {
    TIT_ASSERT(self == nullptr, "`self` must be null for a function!");
    return impl::translate_exceptions<nullptr>([posargs, kwargs]() {
      return impl::invoke<Name, Params...>(Func, posargs, kwargs).release();
    });
  };
  return impl::make_func(Name.c_str(), func_wrapper, module_);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

namespace impl {

// Construct a new method descriptor object from the given function pointer.
auto make_method_descriptor(const char* name,
                            FuncPtr method,
                            const Type& class_) -> Object;

} // namespace impl

/// Method specification.
template<class T, auto Method, class... Params>
concept method_spec =
    std::is_object_v<T> && (param_spec<Params> && ...) &&
    (std::invocable<decltype(Method), T&, typename Params::type...>);

/// Construct a class method descriptor object from the given specification.
template<StrLiteral Name, class T, auto Method, param_spec... Params>
  requires method_spec<T, Method, Params...>
auto make_method_descriptor(const Type& class_) -> Object {
  constexpr impl::FuncPtr method_wrapper =
      [](PyObject* self, PyObject* posargs, PyObject* kwargs) -> PyObject* {
    TIT_ASSERT(self != nullptr, "`self` must not be null for a method!");
    return impl::translate_exceptions<nullptr>([self, posargs, kwargs]() {
      auto func = std::bind_front(Method, std::ref(extract<T>(self)));
      return impl::invoke<Name, Params...>(std::move(func), posargs, kwargs)
          .release();
    });
  };
  return impl::make_method_descriptor(Name.c_str(), method_wrapper, class_);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

namespace impl {

// Python getter function pointer.
using GetPtr = PyObject* (*) (PyObject*, void*);

// Python setter function pointer.
using SetPtr = int (*)(PyObject*, PyObject*, void*);

// Construct a new property descriptor object from the given function pointers.
auto make_prop_descriptor(const char* name,
                          GetPtr get,
                          SetPtr set,
                          const Type& class_) -> Object;

} // namespace impl

/// Property specification.
template<class T, auto Get, auto Set = nullptr>
concept prop_spec =
    std::is_object_v<T> && std::invocable<decltype(Get), T&> &&
    (std::same_as<decltype(Set), std::nullptr_t> ||
     std::
         invocable<decltype(Set), T&, std::invoke_result_t<decltype(Get), T&>>);

/// Construct a class property descriptor object.
template<StrLiteral Name, class T, auto Get, auto Set = nullptr>
  requires prop_spec<T, Get, Set>
auto make_prop_descriptor(const Type& class_) -> Object {
  using Value = std::remove_cvref_t<std::invoke_result_t<decltype(Get), T&>>;
  constexpr impl::GetPtr getter = [](PyObject* self, void* /*closure*/) {
    TIT_ASSERT(self != nullptr, "`self` must not be null for a getter!");
    return impl::translate_exceptions<nullptr>([self]() {
      return object(std::invoke(Get, extract<T>(self))).release();
    });
  };
  constexpr auto setter = [] -> impl::SetPtr {
    if constexpr (!std::same_as<decltype(Set), std::nullptr_t>) {
      return [](PyObject* self, PyObject* value, void* /*closure*/) {
        TIT_ASSERT(self != nullptr, "`self` must not be null for a setter!");
        return impl::translate_exceptions<-1>([self, value]() {
          std::invoke(Set, extract<T>(self), extract<Value>(value));
          return 0;
        });
      };
    }
    return nullptr;
  }();
  return impl::make_prop_descriptor(Name.c_str(), getter, setter, class_);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::py
