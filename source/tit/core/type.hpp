/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <bit>
#include <concepts>
#include <functional>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <typeinfo>
#include <utility>

#include "tit/core/basic_types.hpp"

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Virtual base class.
class VirtualBase {
public:

  /// Default constructor.
  constexpr VirtualBase() = default;

  /// Virtual destructor.
  constexpr virtual ~VirtualBase() = default;

  /// Classes with virtual base should not be move-constructible.
  constexpr VirtualBase(VirtualBase&&) = delete;

  /// Classes with virtual base should not be move-assignable.
  constexpr auto operator=(VirtualBase&&) -> VirtualBase& = delete;

  /// Classes with virtual base should not be copy-constructible.
  constexpr VirtualBase(const VirtualBase&) = delete;

  /// Classes with virtual base should not be copy-assignable.
  constexpr auto operator=(const VirtualBase&) -> VirtualBase& = delete;

}; // class VirtualBase

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Empty and trivial object.
template<class T>
concept empty_type =
    std::is_empty_v<T> && std::is_trivial_v<T> && std::default_initializable<T>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Check if the template type can be constructed from the given arguments.
template<template<class...> class T, class... Args>
concept deduce_constructible_from = requires (Args... args) { T{args...}; };

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

namespace impl {

template<class Type, template<class...> class Class, class... Bound>
inline constexpr bool is_specialization_of_v = false;

template<template<class...> class Class, class... Args>
inline constexpr bool is_specialization_of_v<Class<Args...>, Class> = true;

template<template<class...> class Class, class B1, class... Args>
inline constexpr bool is_specialization_of_v<Class<B1, Args...>, Class, B1> =
    true;

template<template<class...> class Class, class B1, class B2, class... Args>
inline constexpr bool
    is_specialization_of_v<Class<B1, B2, Args...>, Class, B1, B2> = true;

} // namespace impl

/// Check if type is a specialization of the given template.
template<class Type, template<class...> class Class, class... Bound>
concept specialization_of = impl::is_specialization_of_v<Type, Class, Bound...>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

namespace impl {

template<class Tuple>
concept has_tuple_size =
    requires { typename std::tuple_size<Tuple>::type; } &&
    std::derived_from<std::tuple_size<Tuple>,
                      std::integral_constant<size_t, std::tuple_size_v<Tuple>>>;

template<class Tuple, size_t Index>
concept has_tuple_element = requires {
  typename std::tuple_element_t<Index, std::remove_const_t<Tuple>>;
} && requires (Tuple& tuple) {
  {
    std::get<Index>(tuple)
  } -> std::convertible_to<const std::tuple_element_t<Index, Tuple>&>;
};

template<class Tuple, size_t Size>
concept tuple_size_is =
    has_tuple_size<Tuple> && (std::tuple_size_v<Tuple> == Size);

template<class Tuple, size_t Index, class Elem>
concept tuple_element_is =
    has_tuple_element<Tuple, Index> &&
    std::constructible_from<Elem, std::tuple_element_t<Index, Tuple>>;

} // namespace impl

/// Check if a type is tuple-like, i.e., it has `std::tuple_size` and
/// `std::tuple_element` defined, and the `std::get` function is properly
/// defined.
template<class Tuple, class... Items>
concept tuple_like =
    std::is_object_v<Tuple> && impl::has_tuple_size<Tuple> &&
    []<size_t... Indices>(std::index_sequence<Indices...> /*indices*/) {
      return (impl::has_tuple_element<Tuple, Indices> && ...);
    }(std::make_index_sequence<std::tuple_size_v<Tuple>>()) &&
    ((sizeof...(Items) == 0) ||
     (impl::tuple_size_is<Tuple, sizeof...(Items)> &&
      []<size_t... Indices>(std::index_sequence<Indices...> /*indices*/) {
        return (impl::tuple_element_is<Tuple, Indices, Items> && ...);
      }(std::make_index_sequence<sizeof...(Items)>())));

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Type of the difference between two values of the given type.
template<class T>
using difference_t = decltype(std::declval<T>() - std::declval<T>());

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

namespace impl {

template<class T, template<class> class... Fs>
struct composition;

template<class T>
struct composition<T> : std::type_identity<T> {};

template<class T, template<class> class F, template<class> class... Fs>
struct composition<T, F, Fs...> : composition<typename F<T>::type, Fs...> {};

} // namespace impl

/// Compose a set of template modifications.
template<class T, template<class> class... Fs>
using composition_t = typename impl::composition<T, Fs...>::type;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

namespace impl {

template<class T>
struct normalize_type : std::type_identity<T> {};

template<std::unsigned_integral UInt>
  requires (sizeof(UInt) == 1)
struct normalize_type<UInt> : std::type_identity<uint8_t> {};

template<std::unsigned_integral UInt>
  requires (sizeof(UInt) == 2)
struct normalize_type<UInt> : std::type_identity<uint16_t> {};

template<std::unsigned_integral UInt>
  requires (sizeof(UInt) == 4)
struct normalize_type<UInt> : std::type_identity<uint32_t> {};

template<std::unsigned_integral UInt>
  requires (sizeof(UInt) == 8)
struct normalize_type<UInt> : std::type_identity<uint64_t> {};

template<std::floating_point Float>
  requires (sizeof(Float) == 4)
struct normalize_type<Float> : std::type_identity<float32_t> {};

template<std::floating_point Float>
  requires (sizeof(Float) == 8)
struct normalize_type<Float> : std::type_identity<float64_t> {};

// `std::make_signed` and `std::make_unsigned` are normalization-preserving.
template<std::signed_integral SInt>
struct normalize_type<SInt> :
    composition<SInt, std::make_unsigned, normalize_type, std::make_signed> {};

} // namespace impl

/// @brief "Normalize" a type.
///
/// One may explicitly specialize a template for `int64_t`, for example, and
/// assume that it will work for all 64-bit signed integer types. But, that
/// may not work as expected, if, for example, `int64_t` is defined as `long`
/// on some platforms, the specialization for `long long` would be missing.
/// Even if `long` and `long long` have the same size, they are not the same
/// type:
///
/// ```cpp
/// static_assert(sizeof(long) == sizeof(long long) &&
///               !std::same_as<long, long long>);
/// ```
///
/// In a perfect world, the developer must always acknowledge that. But, we
/// cannot always rely on that. For example, Highway has specializations only
/// for standard fixed-width types, like `int64_t`, So, before passing a type
/// to a third-party template, we need to "normalize" it, by replacing it with
/// the standard fixed-width type, if possible.
template<class T>
using normalize_type_t = typename impl::normalize_type<T>::type;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Check that type `T` is in the list `Us...`.
template<class T, class... Us>
inline constexpr bool contains_v = (... || std::is_same_v<T, Us>);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

namespace impl {

template<class T, class U, class... Us>
inline constexpr size_t index_of_v = 1 + index_of_v<T, Us...>;

template<class T, class... Us>
inline constexpr size_t index_of_v<T, T, Us...> = 0;

} // namespace impl

/// Index of type `T` in list `Us...`.
template<class T, class... Us>
  requires contains_v<T, Us...>
inline constexpr auto index_of_v = impl::index_of_v<T, Us...>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

namespace impl {

template<class... Ts>
inline constexpr bool all_unique_v = true;

template<class T, class... Ts>
inline constexpr bool all_unique_v<T, Ts...> =
    (!contains_v<T, Ts...> && all_unique_v<Ts...>);

} // namespace impl

/// Check that all elements in the list `Ts...` are unique.
template<class... Ts>
inline constexpr bool all_unique_v = impl::all_unique_v<Ts...>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Set of unique empty types.
template<empty_type... Ts>
  requires all_unique_v<Ts...>
class TypeSet final {
public:

  /// Construct a set.
  /// @{
  consteval TypeSet() = default;
  consteval explicit TypeSet(Ts... /*elems*/)
    requires (sizeof...(Ts) != 0)
  {}
  /// @}

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Call a function for each element.
  template<class Func>
    requires (std::invocable<Func, Ts> && ...)
  constexpr void for_each([[maybe_unused]] Func func) const {
    (std::invoke(func, Ts{}), ...);
  }

  /// Check if the set contains a type `U`.
  template<empty_type U>
  constexpr auto contains(U /*elem*/) const noexcept -> bool {
    return contains_v<U, Ts...>;
  }

  /// Index of the type in the range.
  template<empty_type U>
    requires contains_v<U, Ts...>
  constexpr auto find(U /*elem*/) const noexcept -> size_t {
    return index_of_v<U, Ts...>;
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Check that @p lhs is a superset of @p rhs.
  template<empty_type... Us>
  friend constexpr auto operator>=(TypeSet /*lhs*/,
                                   TypeSet<Us...> /*rhs*/) noexcept -> bool {
    return (contains_v<Us, Ts...> && ...);
  }

  /// Check that @p lhs is a subset of @p rhs.
  template<empty_type... Us>
  friend constexpr auto operator<=(TypeSet lhs, TypeSet<Us...> rhs) noexcept
      -> bool {
    return rhs >= lhs;
  }

  /// Check that @p lhs is a @b strict superset of @p rhs.
  template<empty_type... Us>
  friend constexpr auto operator>(TypeSet lhs, TypeSet<Us...> rhs) noexcept
      -> bool {
    return (lhs >= rhs) && (sizeof...(Ts) > sizeof...(Us));
  }

  /// Check that @p lhs is a @b strict subset of RHS.
  template<empty_type... Us>
  friend constexpr auto operator<(TypeSet lhs, TypeSet<Us...> rhs) noexcept
      -> bool {
    return rhs > lhs;
  }

  /// Check that @p lhs and @p rhs contain same elements, in any order.
  template<empty_type... Us>
  friend constexpr auto operator==(TypeSet lhs, TypeSet<Us...> rhs) noexcept
      -> bool {
    return (lhs <= rhs) && (lhs >= rhs);
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Set union.
  ///
  /// @returns A set that contains all the elements of @p lhs followed by the
  ///          elements of @p rhs that are not already present in @p lhs. The
  ///          relative order of the elements in both sets is preserved.
  /// @{
  friend constexpr auto operator|(TypeSet lhs, TypeSet<> /*rhs*/) noexcept {
    return lhs;
  }
  template<empty_type U, empty_type... Us>
  friend constexpr auto operator|(TypeSet lhs,
                                  TypeSet<U, Us...> /*rhs*/) noexcept {
    if constexpr (contains_v<U, Ts...>) return lhs | TypeSet<Us...>{};
    else return TypeSet<Ts..., U>{} | TypeSet<Us...>{};
  }
  /// @}

  /// Set intersection.
  ///
  /// @returns A set that contains the elements of @p lhs that are also present
  ///          in @p rhs. The relative order of the elements in LHS is
  ///          preserved.
  /// @{
  friend constexpr auto operator&(TypeSet<> lhs, TypeSet /*rhs*/) noexcept {
    return lhs;
  }
  template<empty_type U, empty_type... Us>
  friend constexpr auto operator&(TypeSet<U, Us...> /*lhs*/,
                                  TypeSet rhs) noexcept {
    if constexpr (!contains_v<U, Ts...>) return TypeSet<Us...>{} & rhs;
    else return TypeSet<U>{} | (TypeSet<Us...>{} & rhs);
  }
  /// @}

  /// Set difference.
  ///
  /// @returns A set that contains all the elements of @p lhs excluding elements
  ///          that are contained in @p rhs. The relative order of the elements
  ///          in LHS is preserved.
  /// @{
  friend constexpr auto operator-(TypeSet<> lhs, TypeSet /*rhs*/) noexcept {
    return lhs;
  }
  template<empty_type U, empty_type... Us>
  friend constexpr auto operator-(TypeSet<U, Us...> /*lhs*/,
                                  TypeSet rhs) noexcept {
    if constexpr (contains_v<U, Ts...>) return (TypeSet<Us...>{} - rhs);
    else return TypeSet<U>{} | (TypeSet<Us...>{} - rhs);
  }
  /// @}

}; // class TypeSet

template<class T>
inline constexpr bool is_type_set_v = false;
template<class... Ts>
inline constexpr bool is_type_set_v<TypeSet<Ts...>> = true;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

namespace impl {

auto demangle(const char* mangled_name) -> std::string;

template<class T>
consteval auto type_name_of_helper() noexcept -> const char* {
  return __PRETTY_FUNCTION__;
}

} // namespace impl

/// Name of the given type (at compile time).
template<class T>
consteval auto type_name_of() noexcept -> std::string_view {
  const std::string_view sample{impl::type_name_of_helper<void>()};
  const auto prefix_size = sample.find("void");
  const auto suffix_size = sample.size() - prefix_size - /*len("void")=*/4;
  std::string_view result{impl::type_name_of_helper<T>()};
  result.remove_prefix(prefix_size), result.remove_suffix(suffix_size);
  return result;
}

/// Name of the given type of the given argument (at runtime).
template<class T>
constexpr auto type_name_of(const T& arg) -> std::string {
  if constexpr (std::is_polymorphic_v<std::remove_cvref_t<T>>) {
    return impl::demangle(typeid(arg).name());
  }
  return std::string{type_name_of<T>()};
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Check if an integer can be safely converted to a different integer type.
template<std::integral To, std::integral Num>
constexpr auto is_safe_cast(Num a) noexcept -> bool {
  return static_cast<Num>(static_cast<To>(a)) == a;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Safely cast a pointer from one type to another between unrelated types.
template<class ToPtr, class FromPtr>
  requires std::is_pointer_v<ToPtr> && std::is_pointer_v<FromPtr> && ([] {
             using T = std::remove_pointer_t<ToPtr>;
             using F = std::remove_pointer_t<FromPtr>;
             return std::is_trivially_copyable_v<T> &&
                    std::is_trivially_copyable_v<F> &&
                    (sizeof(F) % sizeof(T) == 0 || sizeof(T) % sizeof(F) == 0);
           }())
constexpr auto safe_bit_ptr_cast(FromPtr from) noexcept -> ToPtr {
  return std::bit_cast<ToPtr>(from); // NOLINT(bugprone-bitwise-pointer-cast)
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
