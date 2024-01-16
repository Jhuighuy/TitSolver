/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <iostream>

#include "tit/core/posix_utils.hpp"

namespace tit {
namespace {

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

[[gnu::noinline]] void func_3() {
  std::cerr << "func_3\n";
  std::cerr << "Doing something bad...\n";
  int* const null_pointer = nullptr;
  *null_pointer = 0; // NOLINT
}

[[gnu::noinline]] void func_2() {
  std::cerr << "func_2\n";
  func_3();
}

[[gnu::noinline]] void func_1() {
  std::cerr << "func_1\n";
  func_2();
}

void run_test() {
  func_1();
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

} // namespace
} // namespace tit

auto main() -> int {
  using namespace tit;
  const FatalSignalHandler handler{};
  run_test();
  std::cerr << "This line should not be executed.\n";
  return 0;
}
