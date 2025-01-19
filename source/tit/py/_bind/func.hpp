/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// IWYU pragma: private, include "tit/py/bind.hpp"
#pragma once

#include <concepts>
#include <cstddef>
#include <functional>
#include <tuple>
#include <type_traits>
#include <utility>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/str_utils.hpp"

#include "tit/py/core.hpp"

namespace tit::py::cpp {

// NOLINTBEGIN(*-include-cleaner)

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

namespace impl {

// Call the function, and return the result or set the error.
template<auto OnError, std::invocable Func>
  requires std::convertible_to<decltype(OnError), std::invoke_result_t<Func>>
auto safe_call(Func func) noexcept -> std::invoke_result_t<Func> {
  /// @todo Add a proper error handling.
  try {
    return func();
  } catch (const Error& /*e*/) { // NOLINT(*-empty-catch)
    // Error will be reset.
  }
  return OnError;
}

} // namespace impl

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

template<class Self>
struct InstanceParent {
  /// Get the parent object of the instance.
  auto operator()(const Self& /*self*/) const -> Object = delete;
};

namespace impl {

// Is the parent object specified?
template<class Self>
concept has_parent = requires (InstanceParent<Self> getter, Self& self) {
  { getter(self) } -> std::derived_from<Object>;
};

// Increment the reference count of the parent object.
template<class Self>
void incref_parent(const Self& self) {
  if constexpr (has_parent<Self>) {
    constexpr InstanceParent<Self> parent_getter{};
    Py_INCREF(parent_getter(self).get());
  }
}

// Decrement the reference count of the parent object.
template<class Self>
void decref_parent(const Self& self) {
  if constexpr (has_parent<Self>) {
    constexpr InstanceParent<Self> parent_getter{};
    Py_DECREF(parent_getter(self).get());
  }
}

} // namespace impl

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Argument factory.
template<class Type>
using Factory = Type (*)();

/// Default argument specification: either a `nullptr`, a factory function,
/// or a compatible value.
template<class Default, class Type>
concept default_spec = std::same_as<Default, std::nullptr_t> ||
                       std::same_as<Default, Factory<Type>> || //
                       std::same_as<Default, Type>;

namespace impl {

// Convert the default argument to a function.
template<class Type, default_spec<Type> auto Default>
consteval auto make_factory() -> Factory<Type> {
  using D = decltype(Default);
  if constexpr (std::same_as<D, std::nullptr_t>) return nullptr;
  else if constexpr (std::same_as<D, Factory<Type>>) return Default;
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

/// Function specification NTTP.
template<auto Func, class... Params>
concept func_spec = (param_spec<Params> && ...) &&
                    (std::invocable<decltype(Func), typename Params::type...>);

/// Method specification NTTP.
template<auto Method, class Self, class... Params>
concept method_spec =
    std::is_object_v<Self> && (param_spec<Params> && ...) &&
    (std::invocable<decltype(Method), Self&, typename Params::type...>);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

namespace impl {

// Unpack the variadic and keyword arguments into an array.
template<StrLiteral... ParamNames>
auto unpack_args(PyObject* args, PyObject* kwargs)
    -> std::array<Object, sizeof...(ParamNames)> {
  static constexpr auto num_params = sizeof...(ParamNames);
  std::array<Object, num_params> result;
  if (args != nullptr) { // `nullptr` if no positional arguments are given.
    const auto args_ = borrow<Tuple>(args);
    const auto num_args = len(args_);
    if (num_args > num_params) {
      raise_type_error("function takes at most %zi arguments (%zi given)",
                       num_params,
                       num_args);
    }
    for (size_t i = 0; i < num_args; ++i) result[i] = args_[i];
  }
  static constexpr auto param_names = std::to_array<CStrView>({ParamNames...});
  if (kwargs != nullptr) { // `nullptr` if no keyword arguments are given.
    const auto kwargs_ = borrow<Dict>(kwargs);
    kwargs_.for_each([&result](const Object& arg_name, const Object& arg_val) {
      const auto arg_name_str = extract<CStrView>(arg_name);
      const auto param_iter = std::ranges::find(param_names, arg_name_str);
      if (param_iter == param_names.end()) {
        raise_type_error("unexpected argument '%s'", arg_name_str.c_str());
      }
      const auto param_index = std::distance(param_names.begin(), param_iter);
      if (result[param_index].valid()) {
        raise_type_error("duplicate argument '%s'", arg_name_str.c_str());
      }
      result[param_index] = arg_val;
    });
  }
  return result;
}
template<>
inline auto unpack_args(PyObject* args, PyObject* kwargs)
    -> std::array<Object, 0> {
  size_t num_args = 0;
  if (args != nullptr) num_args += len(borrow<Tuple>(args));
  if (kwargs != nullptr) num_args += len(borrow<Dict>(kwargs));
  if (num_args > 0) {
    raise_type_error("function takes no arguments (%zi given)", num_args);
  }
  return std::array<Object, 0>{};
}

// Parse a single argument.
template<param_spec Param>
auto parse_single_arg(const Object& arg) {
  if (!arg.valid()) {
    if constexpr (Param::default_ == nullptr) {
      raise_type_error("missing argument '%s'", Param::name);
    }
    return Param::default_();
  }
  try {
    using Type = typename Param::type;
    if constexpr (!std::is_reference_v<Type>) return extract<Type>(arg);
    else std::ref(extract<Type>(arg));
  } catch (Error& e) {
    e.prefix_message("argument '{}'", Param::name);
    throw;
  }
}

// Parse the function arguments.
template<param_spec... Params>
auto parse_args(PyObject* args, PyObject* kwargs) {
  // Parse the arguments into an array, and then unpack it into a tuple.
  const auto unpacked_args = unpack_args<Params::name...>(args, kwargs);
  return [&]<size_t... Is>(std::index_sequence<Is...> /*indices*/) {
    return std::tuple{parse_single_arg<Params>(unpacked_args[Is])...};
  }(std::make_index_sequence<sizeof...(Params)>{});
}

// Make a `PyMethodDef`.
template<StrLiteral Name, PyCFunctionWithKeywords Func>
constexpr auto make_method_def() noexcept -> PyMethodDef {
  // `_PyCFunction_CAST` prevents us from making this function `consteval`.
  return PyMethodDef{
      .ml_name = Name.c_str(),
      .ml_meth = _PyCFunction_CAST(Func),
      .ml_flags = METH_VARARGS | METH_KEYWORDS,
      .ml_doc = nullptr,
  };
}

} // namespace impl

/// Make a Python function definition.
template<StrLiteral Name, auto Func, param_spec... Params>
  requires func_spec<Func, Params...>
constexpr auto make_func_def() noexcept -> PyMethodDef {
  static constexpr auto Body = [](PyObject* self, //
                                  PyObject* args,
                                  PyObject* kwargs) -> PyObject* {
    TIT_ASSERT(self == nullptr, "`self` must be null for a function!");
    return impl::safe_call<nullptr>([args, kwargs]() {
      auto cppargs = [args, kwargs] {
        try {
          return impl::parse_args<Params...>(args, kwargs);
        } catch (Error& e) {
          e.prefix_message("function '{}'", Name);
          throw;
        }
      }();
      return object(std::apply(Func, std::move(cppargs))).release();
    });
  };
  return impl::make_method_def<Name, Body>();
}

/// Make a Python method definition.
template<StrLiteral Name, auto Method, class Self, param_spec... Params>
  requires method_spec<Method, Self, Params...>
constexpr auto make_method_def() noexcept -> PyMethodDef {
  static constexpr auto Body = [](PyObject* self, //
                                  PyObject* args,
                                  PyObject* kwargs) -> PyObject* {
    TIT_ASSERT(self != nullptr, "`self` must not be null for a method!");
    return impl::safe_call<nullptr>([self, args, kwargs]() {
      auto cppargs = [self, args, kwargs] {
        try {
          return std::tuple_cat(std::tie(extract<Self>(borrow(self))),
                                impl::parse_args<Params...>(args, kwargs));
        } catch (Error& e) {
          e.prefix_message("method '{}'", Name);
          throw;
        }
      }();
      return object(std::apply(Method, std::move(cppargs))).release();
    });
  };
  return impl::make_method_def<Name, Body>();
}

/// Make a Python constructor definition.
template<class Self, param_spec... Params>
// NOLINTNEXTLINE(*-exception-escape) -- False positive.
constexpr auto make_init_def() noexcept -> PyMethodDef {
  static constexpr auto Init = [](Self& self, const Params::type&... args) {
    std::construct_at(std::addressof(self), args...);
    impl::incref_parent(self);
    return None();
  };
  return make_method_def<"__init__", Init, Self, Params...>();
}

/// Make a Python constructor definition that prevents the user from creating
/// instances of the class.
template<class Self, param_spec... Params>
constexpr auto make_noinit_def() noexcept -> PyMethodDef {
  static constexpr auto Init = [](PyObject* self,
                                  PyObject* /*args*/,
                                  PyObject* /*kwargs*/) -> PyObject* {
    const auto self_ = borrow(self);
    /// @todo We need a proper way to format the error message.
    /// @todo Why can't we use `extract<const char*>` here?
    /// @todo Replace an attribute with a full class name.
    PyErr_Format(PyExc_TypeError, // NOLINT(*-vararg)
                 "cannot create '%s' instances",
                 extract<CStrView>(type(self_).attr("__name__")).c_str());
    return nullptr;
  };
  return impl::make_method_def<"__init__", Init>();
}

/// Make a Python destructor function.
template<class Self>
  requires (!std::is_trivially_destructible_v<Self>)
consteval auto make_destructor() noexcept -> destructor {
  return [](PyObject* self) {
    auto& self_ = extract<Self>(borrow(self));
    impl::decref_parent(self_);
    std::destroy_at(std::addressof(self_));
  };
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Getter and setter specification.
template<class Self, auto Get, auto Set = nullptr>
concept getset_spec =
    std::is_object_v<Self> && std::invocable<decltype(Get), Self&> &&
    (std::same_as<decltype(Set), std::nullptr_t> ||
     std::invocable<decltype(Set),
                    Self&,
                    std::invoke_result_t<decltype(Get), Self&>>);

/// Make a Python getter and setter definition.
template<StrLiteral Name, class Self, auto Get, auto Set = nullptr>
consteval auto make_getset_def() noexcept -> PyGetSetDef {
  const getter get = [](PyObject* self, void* /*closure*/) {
    return impl::safe_call<nullptr>([self]() {
      return object(std::invoke(Get, extract<Self>(borrow(self)))).release();
    });
  };
  setter set = nullptr;
  if constexpr (!std::same_as<decltype(Set), std::nullptr_t>) {
    using Prop =
        std::remove_cvref_t<std::invoke_result_t<decltype(Get), Self&>>;
    set = [](PyObject* self, PyObject* value, void* /*closure*/) {
      return impl::safe_call<-1>([self, value]() {
        std::invoke(Set,
                    extract<Self>(borrow(self)),
                    extract<Prop>(borrow(value)));
        return 0;
      });
    };
  }
  return PyGetSetDef{
      .name = Name.c_str(),
      .get = get,
      .set = set,
      .doc = nullptr,
      .closure = nullptr,
  };
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// NOLINTEND(*-include-cleaner)

} // namespace tit::py::cpp
