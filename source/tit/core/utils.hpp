/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <array>
#include <concepts>
#include <utility>

#include "tit/core/basic_types.hpp"

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Wrap a macro argument with commas to pass it to another macro.
#define TIT_PASS(...) __VA_ARGS__

/// Concatenate macro arguments.
#define TIT_CAT(a, b) TIT_CAT_IMPL(a, b)
#define TIT_CAT_IMPL(a, b) a##b

/// Convert macro argument into a string literal.
#define TIT_STR(a) TIT_STR_IMPL(a)
#define TIT_STR_IMPL(a) #a

/// Generate a unique identifier
#define TIT_NAME(prefix) TIT_CAT(TIT_CAT(prefix, _), __LINE__)

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Use this function to assume forwarding references as universal references
/// to avoid false alarms from analysis tools.
/// @{
#define TIT_ASSUME_UNIVERSAL(T, ref) static_cast<void>(std::forward<T>(ref))
#define TIT_ASSUME_UNIVERSALS(Ts, refs) (TIT_ASSUME_UNIVERSAL(Ts, refs), ...)
/// @}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Pack values into a padded array of given size.
template<size_t Size, class T, class... Ts>
  requires ((std::constructible_from<T, Ts &&> && ...) &&
            ((sizeof...(Ts) == Size) ||
             ((sizeof...(Ts) < Size) && std::default_initializable<T>) ))
constexpr auto make_array(Ts&&... vals) -> std::array<T, Size> {
  return {T(std::forward<Ts>(vals))...};
}

/// Fill an array of the given size initialized with the given value.
template<size_t Size, class T>
  requires std::copy_constructible<T>
constexpr auto fill_array(const T& val) -> std::array<T, Size> {
  const auto get_val = [&val](auto /*arg*/) -> const T& { return val; };
  return [&get_val]<size_t... Indices>(std::index_sequence<Indices...>) {
    return std::array<T, Size>{get_val(Indices)...};
  }(std::make_index_sequence<Size>{});
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Virtual base class.
class VirtualBase {
public:

  /// Construct the virtual class instance.
  VirtualBase() = default;

  /// Virtual class instance is not move-constructible.
  VirtualBase(VirtualBase&&) = delete;

  /// Virtual class instance is not copy-constructible.
  VirtualBase(const VirtualBase&) = delete;

  /// Virtual class instance is not movable.
  auto operator=(VirtualBase&&) -> VirtualBase& = delete;

  /// Virtual class instance is not copyable.
  auto operator=(const VirtualBase&) -> VirtualBase& = delete;

  /// Virtual destructor.
  virtual ~VirtualBase() = default;

}; // class VirtualBase

/// Is a virtual class instance of the given type?
template<std::derived_from<VirtualBase> Derived>
constexpr auto instance_of(const VirtualBase& instance) -> bool {
  return dynamic_cast<const Derived*>(&instance) != nullptr;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
