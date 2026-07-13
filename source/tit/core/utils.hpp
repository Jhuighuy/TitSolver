/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <concepts>
#include <cstddef>
#include <memory>

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Wrap a macro argument with commas to pass it to another macro.
#define TIT_PASS(...) __VA_ARGS__

/// Concatenate macro arguments.
#define TIT_CAT(a, b) TIT_CAT_IMPL(a, b)
#define TIT_CAT_IMPL(a, b) a##b

/// Generate a unique identifier
#define TIT_NAME(prefix) TIT_CAT(TIT_CAT(prefix, _), __LINE__)

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Default-initialize a type.
template<std::default_initializable T>
constexpr auto zero(const T& /*a*/) -> T {
  return T{};
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Allocate a new object in a way it does not distress static analysis.
/// @note Use only if unavoidable, e.g., when external libraries require
///       a raw allocation.
template<class T, class... Args>
  requires std::constructible_from<T, Args&&...>
constexpr auto make(Args&&... args) -> T* {
  return std::make_unique<T>(std::forward<Args>(args)...).release();
}

/// Allocate a new array in a way it does not distress static analysis.
/// @note Use only if unavoidable, e.g., when external libraries require
///       a raw allocation.
template<std::default_initializable T>
constexpr auto make_array(std::size_t size) -> T* {
  return std::make_unique<T[]>(size).release(); // NOLINT(*-c-arrays)
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Predicate that is always true.
struct AlwaysTrue final {
  constexpr auto operator()(const auto& /*arg*/) const noexcept -> bool {
    return true;
  }
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
