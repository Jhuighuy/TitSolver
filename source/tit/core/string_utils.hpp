/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License
 * See /LICENSE.md for license information.
 * SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <concepts>
#include <ranges>
#include <string>
#include <string_view>

#include "tit/core/misc.hpp"

namespace tit::string_utils {

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

template<class String>
concept string_like =
    std::same_as<String, char*> || std::same_as<String, const char*> ||
    (std::ranges::random_access_range<String> &&
     std::same_as<std::ranges::range_value_t<String>, char>);

template<class StringRange>
concept string_range = std::ranges::range<StringRange> &&
                       string_like<std::ranges::range_value_t<StringRange>>;

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/** Join the string with delimeter. */
template<string_range Strings>
constexpr auto join(std::string_view with, Strings&& strings) -> std::string {
  TIT_ASSUME_UNIVERSAL(Strings, strings);
  if (strings.size() == 0) return "";
  std::string result(*strings.begin());
  for (const auto& string : strings | std::views::drop(1)) {
    result += with;
    result += string;
  }
  return result;
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

} // namespace tit::string_utils
