/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#define TIT_ENABLE_ASSERTS
#include "tit/core/checks.hpp"
#include "tit/core/env.hpp"
#include "tit/core/main_func.hpp"

namespace tit {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto run_test(CmdArgs /*args*/) -> int {
  // Test string variables.
  TIT_ASSERT(get_env("PATH").has_value(), "Test failed!");
  TIT_ASSERT(!get_env("DOES_NOT_EXIST").has_value(), "Test failed!");

  // Test integer variables.
  TIT_ASSERT(get_env_int("TEST_ZERO") == 0, "Test failed!");
  TIT_ASSERT(get_env_int("TEST_INT") == 123, "Test failed!");
  TIT_ASSERT(get_env_int("TEST_INT", 456) == 123, "Test failed!");
  TIT_ASSERT(get_env_int("TEST_NEGATIVE") == -456, "Test failed!");
  TIT_ASSERT(!get_env_int("DOES_NOT_EXIST").has_value(), "Test failed!");
  TIT_ASSERT(get_env_int("DOES_NOT_EXIST", 456) == 456, "Test failed!");

  // Test unsigned integer variables.
  TIT_ASSERT(get_env_uint("TEST_ZERO") == 0, "Test failed!");
  TIT_ASSERT(get_env_uint("TEST_INT") == 123, "Test failed!");
  TIT_ASSERT(get_env_uint("TEST_INT", 456) == 123, "Test failed!");
  TIT_ASSERT(!get_env_uint("DOES_NOT_EXIST").has_value(), "Test failed!");
  TIT_ASSERT(get_env_uint("DOES_NOT_EXIST", 456) == 456, "Test failed!");

  // Test floating-point variables.
  TIT_ASSERT(get_env_float("TEST_INT") == 123.0, "Test failed!");
  TIT_ASSERT(get_env_float("TEST_FLOAT") == 123.456, "Test failed!");
  TIT_ASSERT(!get_env_float("DOES_NOT_EXIST").has_value(), "Test failed!");
  TIT_ASSERT(get_env_float("DOES_NOT_EXIST", 789.0) == 789.0, "Test failed!");

  // Test boolean variables.
  TIT_ASSERT(get_env_bool("TEST_TRUE").value_or(false), "Test failed!");
  TIT_ASSERT(!get_env_bool("TEST_FALSE").value_or(false), "Test failed!");
  TIT_ASSERT(!get_env_bool("TEST_FALSE", true), "Test failed!");
  TIT_ASSERT(get_env_bool("TEST_INT").value_or(false), "Test failed!");
  TIT_ASSERT(get_env_bool("TEST_INT", false), "Test failed!");
  TIT_ASSERT(!get_env_bool("TEST_ZERO").value_or(false), "Test failed!");
  TIT_ASSERT(!get_env_bool("DOES_NOT_EXIST").has_value(), "Test failed!");
  TIT_ASSERT(get_env_bool("DOES_NOT_EXIST", true), "Test failed!");

  return 0;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit

TIT_IMPLEMENT_MAIN(run_test)
