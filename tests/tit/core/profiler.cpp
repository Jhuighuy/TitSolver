/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <chrono>
#include <thread>

#include "tit/core/basic_types.hpp"
#include "tit/core/cmd.hpp"
#include "tit/core/io.hpp"
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

auto run_test(CmdArgs /*args*/) -> int {
  func_1();
  return 0;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit

TIT_IMPLEMENT_MAIN(run_test)
