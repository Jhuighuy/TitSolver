/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License
 * See /LICENSE.md for license information.
 * SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <utility>

namespace tit {

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/** Use this macro to wrap a macro argument with commas to pass it to another
 ** macro. */
#define TIT_PASS(...) __VA_ARGS__

template<class T>
constexpr void assume_unversal(
    // NOLINTNEXTLINE(cppcoreguidelines-missing-std-forward)
    [[maybe_unused]] T&& universal_reference) noexcept {}

/** Use this function to assume forwarding references as universal references to
 ** avoid false alarms from analysis tools. */
#define TIT_ASSUME_UNIVERSAL(T, universal_reference)                           \
  assume_unversal(std::forward<T>(universal_reference))

template<class... Ts>
constexpr void assume_used(
    // NOLINTNEXTLINE(cppcoreguidelines-missing-std-forward)
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
  // NOLINTNEXTLINE(cppcoreguidelines-c-copy-assignment-signature,misc-unconventional-assign-operator)
  constexpr void operator=(Arg&& arg) {
    func_(std::forward<Arg>(arg));
  }

}; // class OnAssignment

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

} // namespace tit
