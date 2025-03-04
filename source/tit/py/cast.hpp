/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <concepts>
#include <type_traits>

#include "tit/core/str_utils.hpp"

#include "tit/py/error.hpp"
#include "tit/py/number.hpp"
#include "tit/py/object.hpp"
#include "tit/py/sequence.hpp"
#include "tit/py/type.hpp"
#include "tit/py/typing.hpp"

namespace tit::py {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Convert a Python object reference to another type.
template<class To>
struct Cast;

/// @copydoc Cast
template<class To>
inline constexpr auto cast = Cast<To>{};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Convert a Python object reference.
template<std::derived_from<Object> Derived>
struct Cast<Derived> final {
  auto operator()(const Object& obj) const -> Derived {
    static_assert(sizeof(Derived) == sizeof(Object), "Invalid derived type!");
    if (!Derived::isinstance(obj)) {
      raise_type_error("expected '{}', got '{}'",
                       type_name<Derived>(),
                       type(obj).fully_qualified_name());
    }
    return static_cast<const Derived&>(obj);
  }
  template<not_object Value>
  auto operator()(Value&& value) const -> Derived {
    return (*this)(Object{std::forward<Value>(value)});
  }
};

/// Steal the reference to the object expected to be of the given type.
template<std::derived_from<Object> Derived>
auto steal(PyObject* ptr) -> Derived {
  return cast<Derived>(steal(ptr));
}

/// Borrow the reference to the object expected to be of the given type.
template<std::derived_from<Object> Derived>
auto borrow(PyObject* ptr) -> Derived {
  return cast<Derived>(borrow(ptr));
}

/// Maybe steal the reference to the object if it is not null.
template<std::derived_from<Object> Derived = Object>
auto maybe_steal(PyObject* ptr) -> Optional<Derived> {
  if (ptr == nullptr) return None();
  return steal<Derived>(ptr);
}

/// Maybe borrow the reference to the object if it is not null.
template<std::derived_from<Object> Derived = Object>
auto maybe_borrow(PyObject* ptr) -> Optional<Derived> {
  if (ptr == nullptr) return None();
  return borrow<Derived>(ptr);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Convert to C++ bound class reference.
template<class Value>
struct Cast final {
  auto operator()(const Object& obj) const -> Value& {
    using T = std::remove_cvref_t<Value>;
    if (const auto expected_type = impl::lookup_type(typeid(T));
        !type(obj).is_subtype_of(expected_type)) {
      raise_type_error("expected '{}', got '{}'",
                       expected_type.fully_qualified_name(),
                       type(obj).fully_qualified_name());
    }
    return *impl::data<T>(obj.get());
  }
};

/// Convert to C++ boolean value.
template<>
struct Cast<bool> final {
  auto operator()(const Object& obj) const -> bool {
    return cast<Bool>(obj).val();
  }
};

/// Convert to C++ integer value.
template<std::integral Value>
struct Cast<Value> final {
  auto operator()(const Object& obj) const -> Value {
    return static_cast<Value>(cast<Int>(obj).val());
  }
};

/// Convert to C++ floating-point value.
template<std::floating_point Value>
struct Cast<Value> final {
  auto operator()(const Object& obj) const -> Value {
    return static_cast<Value>(cast<Float>(obj).val());
  }
};

/// Convert to C++ string value.
template<str_like Value>
struct Cast<Value> final {
  auto operator()(const Object& obj) const -> Value {
    return static_cast<Value>(cast<Str>(obj).val());
  }
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::py
