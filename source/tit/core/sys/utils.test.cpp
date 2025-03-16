/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "tit/core/sys/utils.hpp"

#include "tit/testing/test.hpp"

namespace tit {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("exe_path") {
  CHECK(exe_path().filename() == "tit_core_tests");
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("get_env") {
  SUBCASE("string") {
    CHECK(get_env("PATH").has_value());
    CHECK_FALSE(get_env("DOES_NOT_EXIST").has_value());
  }
  SUBCASE("int") {
    CHECK(get_env<int>("TEST_ZERO") == 0);
    CHECK(get_env<int>("TEST_POS_INT") == 123);
    CHECK(get_env<int>("TEST_POS_INT", 456) == 123);
    CHECK(get_env<int>("TEST_NEG_INT") == -456);
    CHECK_FALSE(get_env<int>("DOES_NOT_EXIST").has_value());
    CHECK(get_env<int>("DOES_NOT_EXIST", 456) == 456);
    CHECK_FALSE(get_env<int>("TEST_FALSE").has_value());
  }
  SUBCASE("bool") {
    CHECK(get_env<bool>("TEST_TRUE").value_or(false));
    CHECK_FALSE(get_env<bool>("TEST_FALSE").value_or(false));
    CHECK_FALSE(get_env<bool>("TEST_FALSE", true));
    CHECK(get_env<bool>("TEST_POS_INT").value_or(false));
    CHECK(get_env<bool>("TEST_POS_INT", false));
    CHECK_FALSE(get_env<bool>("TEST_ZERO").value_or(false));
    CHECK_FALSE(get_env<bool>("DOES_NOT_EXIST").has_value());
    CHECK(get_env<bool>("DOES_NOT_EXIST", true));
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
