/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <string>
#include <string_view>
#include <vector>

#include <doctest/doctest.h>

#include "tit/core/string_utils.hpp"

namespace tit {
namespace {

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

#define STRING_TYPES const char*, std::string_view, std::string

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

TEST_CASE_TEMPLATE("tit::join_strings", String, STRING_TYPES) {
  // Join a non-empty string list.
  const std::vector<String> strings{
      "budgie", "cockatiel", "cockatoo", "macao", "ringneck",
  };
  CHECK(tit::join_strings(", ", strings) ==
        "budgie, cockatiel, cockatoo, macao, ringneck");
  // Join an empty string list.
  const std::vector<String> empty_strings{};
  CHECK(tit::join_strings("...", empty_strings) == "");
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

} // namespace
} // namespace tit
