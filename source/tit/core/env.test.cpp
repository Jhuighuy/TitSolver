/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "tit/core/env.hpp"
#include "tit/core/exception.hpp"

#include "tit/testing/test.hpp"

namespace tit {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("get_env") {
  CHECK(get_env("PATH").has_value());
  CHECK(get_env("HOME").has_value());
  CHECK_FALSE(get_env("DOES_NOT_EXIST").has_value());
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("set_env") {
  SUBCASE("string") {
    SUBCASE("valid") {
      set_env("TEST_VAR", "TEST");
      CHECK(get_env("TEST_VAR") == "TEST");
    }
    SUBCASE("invalid") {
      CHECK_THROWS_MSG(
          set_env("TEST=VAR", "TEST"),
          Exception,
          "Unable to set environment variable 'TEST=VAR' value to 'TEST'.");
    }
  }
  SUBCASE("int") {
    SUBCASE("positive") {
      set_env("TEST_VAR", 123);
      CHECK(get_env<int>("TEST_VAR") == 123);
    }
    SUBCASE("negative") {
      set_env("TEST_VAR", -123);
      CHECK(get_env<int>("TEST_VAR") == -123);
    }
    SUBCASE("invalid") {
      set_env("TEST_VAR", "TEST");
      CHECK_THROWS_MSG(get_env<int>("TEST_VAR"),
                       Exception,
                       "Unable to convert the environment variable 'TEST_VAR' "
                       "value 'TEST' to 'int'.");
    }
  }
  SUBCASE("bool") {
    SUBCASE("false") {
      set_env("TEST_VAR", "false");
      CHECK(get_env<bool>("TEST_VAR") == false);
    }
    SUBCASE("true") {
      set_env("TEST_VAR", "true");
      CHECK(get_env<bool>("TEST_VAR") == true);
    }
    SUBCASE("invalid") {
      set_env("TEST_VAR", "TEST");
      CHECK_THROWS_MSG(get_env<bool>("TEST_VAR"),
                       Exception,
                       "Unable to convert the environment variable 'TEST_VAR' "
                       "value 'TEST' to 'bool'.");
    }
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("unset_env") {
  set_env("TEST_VAR", "TEST");
  REQUIRE(get_env("TEST_VAR") == "TEST");
  unset_env("TEST_VAR");
  CHECK_FALSE(get_env("TEST_VAR").has_value());
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
