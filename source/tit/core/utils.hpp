/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <concepts>
#include <functional>
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

/// Predicate that is always true.
struct AlwaysTrue final {
  constexpr auto operator()(const auto& /*arg*/) const noexcept -> bool {
    return true;
  }
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
