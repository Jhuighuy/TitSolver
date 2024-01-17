/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <stdio.h> // NOLINT(*-deprecated-headers)

#include "tit/core/compat.hpp"
#include "tit/core/main_func.hpp"

namespace tit {
namespace {

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

[[gnu::noinline]] void func_3() {
  Std::println(stderr, "func_3");
  Std::println(stderr, "Doing something bad...");
  int* const null_pointer = nullptr;
  *null_pointer = 0; // NOLINT
}

[[gnu::noinline]] void func_2() {
  Std::println(stderr, "func_2");
  func_3();
}

[[gnu::noinline]] void func_1() {
  Std::println(stderr, "func_1");
  func_2();
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

auto run_test(int /*argc*/, char** /*argv*/) -> int {
  func_1();
  Std::println(stderr, "This line should not be executed.");
  return 0;
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

} // namespace
} // namespace tit

auto main(int argc, char** argv) -> int {
  using namespace tit;
  run_main(argc, argv, &run_test);
}
