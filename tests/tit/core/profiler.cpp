/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <chrono>
#include <thread>

#include "tit/core/compat.hpp"
#include "tit/core/profiler.hpp"
#include "tit/core/types.hpp"

namespace tit {
namespace {

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

[[gnu::noinline]] void func_3() {
  TIT_PROFILE_SECTION("func_3");
  Std::println("func_3");
  std::this_thread::sleep_for(std::chrono::microseconds(10));
}

[[gnu::noinline]] void func_2() {
  TIT_PROFILE_SECTION("func_2");
  Std::println("func_2");
  for (size_t i = 0; i < 3; ++i) func_3();
  std::this_thread::sleep_for(std::chrono::microseconds(20));
}

[[gnu::noinline]] void func_1() {
  TIT_PROFILE_SECTION("func_1");
  Std::println("func_1");
  for (size_t i = 0; i < 3; ++i) func_2();
  std::this_thread::sleep_for(std::chrono::microseconds(40));
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

} // namespace
} // namespace tit

auto main() -> int {
  using namespace tit;
  Profiler::enable();
  func_1();
  return 0;
}
