/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "tit/core/checks.hpp"
#include "tit/core/env.hpp"
#include "tit/core/main_func.hpp"

namespace tit {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto run_test(CmdArgs /*args*/) -> int {
  // Test string variables.
  TIT_ENSURE(get_env("PATH").has_value(), "");
  TIT_ENSURE(!get_env("DOES_NOT_EXIST").has_value(), "");

  // Test integer variables.
  TIT_ENSURE(get_env_int("TEST_ZERO") == 0, "");
  TIT_ENSURE(get_env_int("TEST_INT") == 123, "");
  TIT_ENSURE(get_env_int("TEST_INT", 456) == 123, "");
  TIT_ENSURE(get_env_int("TEST_NEGATIVE") == -456, "");
  TIT_ENSURE(!get_env_int("DOES_NOT_EXIST").has_value(), "");
  TIT_ENSURE(get_env_int("DOES_NOT_EXIST", 456) == 456, "");

  // Test unsigned integer variables.
  TIT_ENSURE(get_env_uint("TEST_ZERO") == 0, "");
  TIT_ENSURE(get_env_uint("TEST_INT") == 123, "");
  TIT_ENSURE(get_env_uint("TEST_INT", 456) == 123, "");
  TIT_ENSURE(!get_env_uint("DOES_NOT_EXIST").has_value(), "");
  TIT_ENSURE(get_env_uint("DOES_NOT_EXIST", 456) == 456, "");

  // Test floating-point variables.
  TIT_ENSURE(get_env_float("TEST_INT") == 123.0, "");
  TIT_ENSURE(get_env_float("TEST_FLOAT") == 123.456, "");
  TIT_ENSURE(!get_env_float("DOES_NOT_EXIST").has_value(), "");
  TIT_ENSURE(get_env_float("DOES_NOT_EXIST", 789.0) == 789.0, "");

  // Test boolean variables.
  TIT_ENSURE(get_env_bool("TEST_TRUE").value_or(false), "");
  TIT_ENSURE(!get_env_bool("TEST_FALSE").value_or(false), "");
  TIT_ENSURE(!get_env_bool("TEST_FALSE", true), "");
  TIT_ENSURE(get_env_bool("TEST_INT").value_or(false), "");
  TIT_ENSURE(get_env_bool("TEST_INT", false), "");
  TIT_ENSURE(!get_env_bool("TEST_ZERO").value_or(false), "");
  TIT_ENSURE(!get_env_bool("DOES_NOT_EXIST").has_value(), "");
  TIT_ENSURE(get_env_bool("DOES_NOT_EXIST", true), "");

  return 0;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit

TIT_IMPLEMENT_MAIN(run_test)
