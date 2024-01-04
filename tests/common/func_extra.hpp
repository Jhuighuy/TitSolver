/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <concepts> // IWYU pragma: keep
#include <functional>
#include <utility>

#include "tit/core/types.hpp"

namespace tit {

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/** A wrapper for a function with call counter. */
template<class Func>
class CountedFunc {
public:

  /* Initialize a wrapper with a specified function. */
  constexpr explicit CountedFunc(Func func) noexcept : func_(std::move(func)) {}

  /** Call the function and increase the call counter. */
  template<class... Args>
    requires std::invocable<Func, Args&&...>
  constexpr auto operator()(Args&&... args) {
    count_ += 1;
    return std::invoke(func_, std::forward<Args>(args)...);
  }

  /** Call count. */
  constexpr auto count() const noexcept {
    return count_;
  }

private:

  Func func_;
  size_t count_ = 0;

}; // class CountedFunc

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

} // namespace tit
