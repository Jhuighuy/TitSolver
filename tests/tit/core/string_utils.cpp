/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License
 * See /LICENSE.md for license information.
 * SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <string>
#include <vector>

#include <doctest/doctest.h>

#include "tit/core/string_utils.hpp"

namespace {

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

TEST_CASE("tit::core::string_utils::join") {
  // Join a non-empty string list.
  const std::vector<std::string> strings{
      "budgie", "cockatiel", "cockatoo", "macao", "ringneck",
  };
  CHECK(tit::string_utils::join(", ", strings) ==
        "budgie, cockatiel, cockatoo, macao, ringneck");
  // Join an empty string list.
  const std::vector<std::string> empty_strings{};
  CHECK(tit::string_utils::join("...", empty_strings) == "");
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

} // namespace
