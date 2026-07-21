/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <concepts>
#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <utility>

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

/// Overloaded function object.
template<class... Funcs>
struct Overload final : Funcs... {
  using Funcs::operator()...;
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Call a function on scope exit.
template<std::invocable Func>
class Defer final {
public:

  /// Construct a scope-exit callback.
  constexpr explicit Defer(Func func) noexcept : func_{std::move(func)} {}

  /// Move-construct a scope-exit callback.
  constexpr Defer(Defer&& other) noexcept : func_{std::move(other.func_)} {}

  /// Scope-exit callback is not copy-constructible.
  constexpr Defer(const Defer&) = delete;

  /// Move-assign a scope-exit callback.
  constexpr auto operator=(Defer&& other) noexcept -> Defer& {
    func_ = std::move(other.func_);
    return *this;
  }

  /// Scope-exit callback is not copy-assignable.
  constexpr auto operator=(const Defer&) -> Defer& = delete;

  /// Run the stored callback, unless it was canceled.
  constexpr ~Defer() {
    if (func_.has_value()) std::invoke(func_.value());
  }

  /// Cancel the stored callback.
  constexpr void cancel() noexcept {
    func_.reset();
  }

private:

  std::optional<Func> func_;

}; // class Defer

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
