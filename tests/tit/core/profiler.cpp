/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <chrono>
#include <thread>

#include "tit/core/basic_types.hpp"
#include "tit/core/io.hpp"
#include "tit/core/main_func.hpp"
#include "tit/core/profiler.hpp"

namespace tit {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

[[gnu::noinline]] void func_3() {
  TIT_PROFILE_SECTION("func_3");
  println("func_3");
  std::this_thread::sleep_for(std::chrono::microseconds(10));
}

[[gnu::noinline]] void func_2() {
  TIT_PROFILE_SECTION("func_2");
  println("func_2");
  for (size_t i = 0; i < 3; ++i) func_3();
  std::this_thread::sleep_for(std::chrono::microseconds(20));
}

[[gnu::noinline]] void func_1() {
  TIT_PROFILE_SECTION("func_1");
  println("func_1");
  for (size_t i = 0; i < 3; ++i) func_2();
  std::this_thread::sleep_for(std::chrono::microseconds(40));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto run_test(int /*argc*/, char** /*argv*/) -> int {
  func_1();
  return 0;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit

auto main(int argc, char** argv) -> int {
  using namespace tit;
  run_main(argc, argv, &run_test);
  return 0;
}
