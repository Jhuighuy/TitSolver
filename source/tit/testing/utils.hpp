/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <chrono>
#include <concepts>
#include <functional>
#include <thread>

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// A wrapper for a function that sleeps for a given amount of time.
template<class Func>
class SleepFunc final {
public:

  // Initialize a sleep function.
  constexpr explicit SleepFunc(Func func,
                               std::chrono::milliseconds duration =
                                   std::chrono::milliseconds(10)) noexcept
      : func_{std::move(func)}, duration_{duration} {}

  // Sleep for the specified amount of time and call the function.
  template<class... Args>
    requires std::invocable<Func&, Args&&...>
  constexpr auto operator()(Args&&... args) const -> decltype(auto) {
    std::this_thread::sleep_for(duration_);
    return std::invoke(func_, std::forward<Args>(args)...);
  }

private:

  Func func_;
  std::chrono::milliseconds duration_;

}; // class SleepFunc

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
