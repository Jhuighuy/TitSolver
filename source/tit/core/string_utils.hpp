/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <concepts> // IWYU pragma: keep
#include <ranges>
#include <string>
#include <string_view>

#include "tit/core/misc.hpp"

namespace tit {

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/** A string type: `const char*`, `std::string`, `std::string_view`, etc. */
template<class String>
concept string = std::constructible_from<std::string_view, String>;

/** A range of strings. */
template<class Strings>
concept string_range =
    std::ranges::range<Strings> && string<std::ranges::range_value_t<Strings>>;

/** An input range of strings. */
template<class Strings>
concept input_string_range =
    string_range<Strings> && std::ranges::input_range<Strings>;

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/** Join the string with delimiter. */
template<input_string_range Strings>
constexpr auto join_strings(std::string_view with, Strings&& strings)
    -> std::string {
  TIT_ASSUME_UNIVERSAL(Strings, strings);
  if (std::ranges::empty(strings)) return "";
  std::string result(*strings.begin());
  for (const auto& string : strings | std::views::drop(1)) {
    result += with;
    result += string;
  }
  return result;
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

} // namespace tit
