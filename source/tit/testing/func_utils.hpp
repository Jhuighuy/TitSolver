// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Part of the Tit Solver project, under the MIT License.
// See /LICENSE.md for license information. SPDX-License-Identifier: MIT
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#pragma once

#include <chrono>
#include <concepts>
#include <functional>
#include <thread>
#include <utility>

#include "tit/core/basic_types.hpp"

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// A wrapper for a function with call counter.
template<class Func>
class CountedFunc {
public:

  /// Initialize a wrapper with a specified function.
  constexpr explicit CountedFunc(Func func) noexcept : func_(std::move(func)) {}

  /// Call the function and increase the call counter.
  template<class... Args>
    requires std::invocable<Func&, Args&&...>
  constexpr auto operator()(Args&&... args) {
    count_ += 1;
    return std::invoke(func_, std::forward<Args>(args)...);
  }

  /// Call count.
  constexpr auto count() const noexcept {
    return count_;
  }

private:

  Func func_;
  size_t count_ = 0;

}; // class CountedFunc

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// A wrapper for a function that sleeps for a given amount of time.
template<class Func>
class SleepFunc final {
public:

  /// Initialize a sleep function.
  constexpr explicit SleepFunc(Func func,
                               std::chrono::milliseconds duration =
                                   std::chrono::milliseconds(10)) noexcept
      : func_{std::move(func)}, duration_{duration} {}

  /// Sleep for the specified amount of time and call the function.
  template<class... Args>
    requires std::invocable<Func&, Args&&...>
  constexpr auto operator()(Args&&... args) const -> decltype(auto) {
    std::this_thread::sleep_for(duration_);
    return func_(std::forward<Args>(args)...);
  }

private:

  Func func_;
  std::chrono::milliseconds duration_;

}; // class SleepFunc

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
