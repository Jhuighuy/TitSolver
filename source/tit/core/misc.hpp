/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <array>
#include <concepts> // IWYU pragma: keep
#include <cstdlib>
#include <utility>

#include "tit/core/config.hpp"
#include "tit/core/types.hpp"

#if TIT_GCOV
extern "C" void __gcov_dump(); // NOLINT(*-reserved-identifier,cert-*)
#endif

namespace tit {

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/** Wrap a macro argument with commas to pass it to another macro. */
#define TIT_PASS(...) __VA_ARGS__

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/** Use this function to assume forwarding references as universal references
 ** to avoid false alarms from analysis tools. */
/** @{ */
#define TIT_ASSUME_UNIVERSAL(T, ref) static_cast<void>(std::forward<T>(ref))
#define TIT_ASSUME_UNIVERSALS(Ts, refs) (TIT_ASSUME_UNIVERSAL(Ts, refs), ...)
/** @} */

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/** Pack values into an a padded array of given size. */
template<size_t Size, class T, class... Ts>
  requires (std::convertible_to<Ts, T> && ...) &&
           ((sizeof...(Ts) == Size) ||
            ((sizeof...(Ts) <= Size) && std::default_initializable<T>) )
constexpr auto pack(Ts&&... values) noexcept -> std::array<T, Size> {
  return {static_cast<T>(std::forward<Ts>(values))...};
}

/** Convenience function used inside of some macros. */
/** @{ */
template<class T>
constexpr auto _unwrap(T&& value) noexcept -> decltype(auto) {
  return std::forward<T>(value);
}
template<class T>
constexpr auto _unwrap(T* value) noexcept -> T& {
  return *value;
}
/** @} */

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/** Exit from the current process. */
[[noreturn]] inline void exit(int exit_code) noexcept {
  std::exit(exit_code); // NOLINT(concurrency-mt-unsafe)
}

/** Fast-exit from the current process. */
[[noreturn]] inline void fast_exit(int exit_code) noexcept {
#if TIT_GCOV
  __gcov_dump();
#endif
  std::_Exit(exit_code);
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

} // namespace tit
