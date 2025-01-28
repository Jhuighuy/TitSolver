/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
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
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/str_utils.hpp"

#include "tit/py/cast.hpp"
#include "tit/py/error.hpp"
#include "tit/py/mapping.hpp"
#include "tit/py/object.hpp"
#include "tit/py/type.hpp"

namespace tit::py {

class Module;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Argument factory.
template<class Type>
using Factory = Type (*)();

/// Default argument specification: either a `nullptr`, a factory function,
/// or a compatible value.
template<class Default, class Type>
concept default_spec = std::same_as<Default, std::nullptr_t> ||
                       std::convertible_to<Default, Factory<Type>> || //
                       std::same_as<Default, Type>;

namespace impl {

// Convert the default argument to a function.
template<class Type, default_spec<Type> auto Default>
consteval auto make_factory() -> Factory<Type> {
  using D = decltype(Default);
  if constexpr (std::same_as<D, std::nullptr_t>) return nullptr;
  else if constexpr (std::convertible_to<D, Factory<Type>>) return Default;
  else if constexpr (std::same_as<D, Type>) return [] { return Default; };
  else static_assert(false);
}

} // namespace impl

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Function parameter specification.
template<class Type, StrLiteral Name, Factory<Type> Default>
struct ParamSpec final {
  using type = Type;                        ///< Parameter type.
  static constexpr auto name = Name;        ///< Parameter name.
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
inline auto count_args(PyObject* posargs, PyObject* kwargs) -> size_t {
  TIT_ASSERT(posargs != nullptr, "Positional arguments must not be null!");
  auto result = len(borrow<Tuple>(posargs));
  if (kwargs != nullptr) result += len(borrow<Dict>(kwargs));
  return result;
}

// Unpack the variadic and keyword arguments into an array.
template<StrLiteral... ParamNames>
auto unpack_args(PyObject* posargs, PyObject* kwargs)
    -> std::array<Object, sizeof...(ParamNames)> {
  static constexpr auto num_params = sizeof...(ParamNames);
  std::array<Object, num_params> result;

  // Parse positional arguments.
  TIT_ASSERT(posargs != nullptr, "Positional arguments must not be null!");
  const auto posargs_ = borrow<Tuple>(posargs);
  const auto num_posargs = len(posargs_);
  if (num_posargs > num_params) {
    raise_type_error("function takes at most {} arguments ({} given)",
                     num_params,
                     count_args(posargs, kwargs));
  }
  for (size_t i = 0; i < num_posargs; ++i) result[i] = posargs_[i];

  // Parse keyword arguments.
  static constexpr auto param_names = std::to_array<CStrView>({ParamNames...});
  if (kwargs != nullptr) {
    const auto kwargs_ = borrow<Dict>(kwargs);
    kwargs_.for_each([&result](const Object& arg_name, const Object& arg_val) {
      const auto arg_name_str = extract<CStrView>(arg_name);
      const auto param_iter = std::ranges::find(param_names, arg_name_str);
      if (param_iter == param_names.end()) {
        raise_type_error("unexpected argument '{}'", arg_name_str);
      }
      const auto param_index = std::distance(param_names.begin(), param_iter);
      if (result[param_index].valid()) {
        raise_type_error("duplicate argument '{}'", arg_name_str);
      }
      result[param_index] = arg_val;
    });
  }

  return result;
}
template<>
inline auto unpack_args(PyObject* posargs, PyObject* kwargs)
    -> std::array<Object, 0> {
  if (const auto num_args = count_args(posargs, kwargs); num_args > 0) {
    raise_type_error("function takes no arguments ({} given)", num_args);
  }
  return std::array<Object, 0>{};
}

// Parse a single argument.
template<param_spec Param>
auto parse_single_arg(const Object& arg) {
  // Fill the default argument value.
  if (!arg.valid()) {
    if constexpr (Param::default_ == nullptr) {
      raise_type_error("missing argument '{}'", Param::name);
    }
    return Param::default_();
  }

  // Extract the argument value.
  try {
    using ParamType = typename Param::type;
    return ParamType{extract<ParamType>(arg)};
  } catch (ErrorException& e) {
    e.prefix_message("argument '{}'", Param::name);
    throw;
  }
}

// Parse the function arguments.
template<param_spec... Params>
auto parse_args(PyObject* posargs, PyObject* kwargs) {
  // Parse the arguments into an array, and then unpack it into a tuple.
  const auto unpacked_args = unpack_args<Params::name...>(posargs, kwargs);
  return [&]<size_t... Is>(std::index_sequence<Is...> /*indices*/) {
    return std::tuple{parse_single_arg<Params>(unpacked_args[Is])...};
  }(std::make_index_sequence<sizeof...(Params)>{});
}

// Call the function, and return the result as a Python object.
template<auto Func, class ArgsTuple>
auto call_func(ArgsTuple&& args) -> Object {
  using Result = decltype(std::apply(Func, std::forward<ArgsTuple>(args)));
  if constexpr (std::same_as<Result, void>) {
    std::apply(Func, std::forward<ArgsTuple>(args));
    return None();
  } else {
    return object(std::apply(Func, std::forward<ArgsTuple>(args)));
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

/// Function specification NTTP.
template<auto Func, class... Params>
concept func_spec = (param_spec<Params> && ...) &&
                    (std::invocable<decltype(Func), typename Params::type...>);

/// C++ function pointer.
using CppFuncPtr = PyObject* (*) (PyObject*, PyObject*, PyObject*);

/// Make a Python function pointer.
template<StrLiteral Name, auto Func, param_spec... Params>
  requires func_spec<Func, Params...>
consteval auto make_func_ptr() noexcept -> CppFuncPtr {
  return [](PyObject* self, PyObject* posargs, PyObject* kwargs) -> PyObject* {
    TIT_ASSERT(self == nullptr, "`self` must be null for a function!");
    return impl::translate_exceptions<nullptr>([posargs, kwargs]() {
      // Parse the arguments.
      auto args = [posargs, kwargs] {
        try {
          return impl::parse_args<Params...>(posargs, kwargs);
        } catch (ErrorException& e) {
          e.prefix_message("function '{}'", Name);
          throw;
        }
      }();

      // Call the function.
      return impl::call_func<Func>(std::move(args)).release();
    });
  };
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Python C++ function object reference.
class CFunction final : public Object {
public:

  /// Get the type object of the `CFunction`.
  static auto type() -> Type;

  /// Check if the object is a subclass of `CFunction`.
  static auto isinstance(const Object& obj) -> bool;

  /// Construct a new C++ function object from a function pointer.
  CFunction(std::string name, CppFuncPtr func, const Module* module_ = nullptr);

}; // class CFunction

/// Make a C++ function object.
template<StrLiteral Name, auto Func, param_spec... Params>
  requires func_spec<Func, Params...>
auto make_func(const Module* module_ = nullptr) -> CFunction {
  return CFunction{std::string{Name},
                   make_func_ptr<Name, Func, Params...>(),
                   module_};
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::py
