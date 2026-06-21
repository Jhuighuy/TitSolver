/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <cstddef>
#include <string>

#include "tit/prop/path.hpp"
#include "tit/testing/test.hpp"

namespace tit {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("prop::Path") {
  SUBCASE("root") {
    const prop::Path path;
    CHECK(path.empty());
    CHECK(path.string() == "/");
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("prop::Path::child") {
  const auto path = prop::Path{}.child("materials").child(2).child("id");
  CHECK(path.string() == "/materials/2/id");
  CHECK_FALSE(path.empty());
  REQUIRE(path.size() == 3);
  CHECK(std::get<std::string>(path[0]) == "materials");
  CHECK(std::get<std::size_t>(path[1]) == 2);
  CHECK(std::get<std::string>(path[2]) == "id");
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("prop::Path::parent") {
  const auto path = prop::Path{}.child("materials").child(2).child("id");
  CHECK(path.parent().string() == "/materials/2");
  CHECK(path.parent().parent().string() == "/materials");
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
