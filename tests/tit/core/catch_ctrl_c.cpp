/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <cstdlib>
#include <iostream>

#include <signal.h> // NOLINT(*-deprecated-headers)

#include "tit/core/assert.hpp"
#include "tit/core/posix_utils.hpp"

namespace tit {
namespace {

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

[[gnu::noinline]] void func_3() {
  std::cerr << "func_3\n";
  std::cerr << "Simulating Ctrl+C...\n";
  const auto status = raise(SIGINT);
  TIT_ENSURE(status == 0, "Failed to raise an interrupt.");
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

void setup_exit_function() {
  const auto status = std::atexit([] { std::cerr << "At exit...\n"; });
  TIT_ENSURE(status == 0, "Failed to register an exit function.");
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

} // namespace
} // namespace tit

auto main() -> int {
  using namespace tit;
  const FatalSignalHandler handler{};
  setup_exit_function();
  run_test();
  std::cerr << "This line should not be executed.\n";
  return 0;
}
