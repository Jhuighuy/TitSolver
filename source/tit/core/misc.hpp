/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <array>
#include <concepts>
#include <utility>

#include "tit/core/types.hpp"

namespace tit {

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/** Use this macro to wrap a macro argument with commas to pass it to another
 ** macro. */
#define TIT_PASS(...) __VA_ARGS__

template<class... T>
constexpr void assume_universal(
    // NOLINTNEXTLINE(*-missing-std-forward)
    [[maybe_unused]] T&&... universal_references) noexcept {}

/** Use this function to assume forwarding references as universal references to
 ** avoid false alarms from analysis tools. */
/** @{ */
#define TIT_ASSUME_UNIVERSAL(T, universal_reference)                           \
  assume_universal(std::forward<T>(universal_reference))
#define TIT_ASSUME_UNIVERSALS(Ts, universal_references)                        \
  assume_universal(std::forward<Ts>(universal_references)...)
/** @} */

template<class... Ts>
constexpr void assume_used(
    // NOLINTNEXTLINE(*-missing-std-forward)
    [[maybe_unused]] Ts&&... args) noexcept {}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

template<class Instance, template<class...> class Template>
inline constexpr bool is_specialization_of_v = //
    false;
template<class... Args, template<class...> class Template>
inline constexpr bool is_specialization_of_v<Template<Args...>, Template> =
    true;

/** Check if `Instance` is a specialization of the template class `Template`. */
template<class Instance, template<class...> class Template>
concept specialization_of = is_specialization_of_v<Instance, Template>;

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

template<class Func>
class OnAssignment {
private:

  Func func_;

public:

  constexpr explicit OnAssignment(Func func) : func_{std::move(func)} {}

  template<class Arg>
  // NOLINTNEXTLINE(*-copy-assignment-signature,*-unconventional-assign-operator)
  constexpr void operator=(Arg&& arg) {
    func_(std::forward<Arg>(arg));
  }

}; // class OnAssignment

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/** Check that value @p x is in range [@p a, @p b].*/
template<class T>
constexpr auto in_range(const T& a, const T& x, const T& b) noexcept -> bool {
  return a <= x && x <= b;
}

/** Check that value @p X is in range [@p A, @p B].*/
template<auto A, auto X, auto B>
  requires (std::same_as<decltype(X), decltype(A)> &&
            std::same_as<decltype(X), decltype(B)>)
inline constexpr bool in_range_v = in_range(A, X, B);

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/** Pack values into an a padded array of given size. */
template<size_t Size, class T, class... Ts>
  requires (std::convertible_to<Ts, T> && ...) &&
           ((sizeof...(Ts) == Size) ||
            ((sizeof...(Ts) <= Size) && std::default_initializable<T>) )
constexpr auto pack(Ts&&... values) noexcept -> std::array<T, Size> {
  return std::array<T, Size>{static_cast<T>(std::forward<Ts>(values))...};
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

// Convenience function used inside of some macros.
template<class T>
constexpr auto _unwrap(T&& value) noexcept -> decltype(auto) {
  return std::forward<T>(value);
}
template<class T>
constexpr auto _unwrap(T* value) noexcept -> T& {
  return *value;
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

} // namespace tit
