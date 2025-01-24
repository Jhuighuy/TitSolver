/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <concepts>
#include <string_view>
#include <type_traits>
#include <utility>

#include "tit/core/str_utils.hpp"
#include "tit/core/type_traits.hpp"

namespace tit::py {

class Object;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

namespace impl {

// Converter from C++ values to Python objects and back.
template<class Value>
struct Converter;

// Boolean converter.
template<>
struct Converter<bool> final {
  static auto object(bool value) -> Object;
  static auto extract(const Object& obj) -> bool;
};

// Integer converter.
template<>
struct Converter<long long> final {
  static auto object(long long value) -> Object;
  static auto extract(const Object& obj) -> long long;
};
template<std::integral Value>
struct Converter<Value> final {
  using Base = Converter<defer_t<long long, Value>>;
  static auto object(Value value) -> defer_t<Object, Value> {
    return Base::object(static_cast<long long>(value));
  }
  static auto extract(const Object& obj) -> Value {
    return static_cast<Value>(Base::extract(obj));
  }
};

// Floating-point value converter.
template<>
struct Converter<double> final {
  static auto object(double value) -> Object;
  static auto extract(const Object& obj) -> double;
};
template<std::floating_point Value>
struct Converter<Value> final {
  using Base = Converter<defer_t<double, Value>>;
  static auto object(Value value) -> defer_t<Object, Value> {
    return Base::object(static_cast<double>(value));
  }
  static auto extract(const Object& obj) -> Value {
    return static_cast<Value>(Base::extract(obj));
  }
};

// String converter.
template<>
struct Converter<std::string_view> final {
  static auto object(std::string_view value) -> Object;
  static auto extract(const Object& obj) -> CStrView;
};
template<str_like Value>
struct Converter<Value> final {
  using Base = Converter<defer_t<std::string_view, Value>>;
  static auto object(const Value& value) -> defer_t<Object, Value> {
    return Base::object(value);
  }
  static auto extract(const Object& obj) -> Value {
    return static_cast<Value>(Base::extract(obj));
  }
};

// Python object converter.
template<std::derived_from<Object> Derived>
struct Converter<Derived> final {
  static auto object(const Derived& obj) -> defer_t<Object, Derived> {
    return obj; // Already a Python object.
  }
  static auto extract(const Object& obj) -> Derived {
    return expect<Derived>(obj);
  }
};

} // namespace impl

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Make a Python object from the given argument.
template<class Value>
auto object(Value&& value) -> defer_t<Object, Value> {
  return impl::Converter<std::decay_t<Value>>::object(
      std::forward<Value>(value));
}

/// Extract the C++ value from the Python object.
template<class Value>
auto extract(const Object& obj) -> decltype(auto) {
  return impl::Converter<std::decay_t<Value>>::extract(obj);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::py
