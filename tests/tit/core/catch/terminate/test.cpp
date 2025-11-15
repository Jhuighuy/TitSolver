/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <chrono>
#include <thread>

#include "tit/core/main.hpp"
#include "tit/core/print.hpp"

namespace tit {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

[[gnu::noinline]] void func_3() {
  eprintln("func_3");
  eprintln("Creating a joinable thread...");
  std::thread(
      [] { std::this_thread::sleep_for(std::chrono::milliseconds(100)); });
}

[[gnu::noinline]] void func_2() {
  eprintln("func_2");
  func_3();
}

[[gnu::noinline]] void func_1() {
  eprintln("func_1");
  func_2();
}

} // namespace
} // namespace tit

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TIT_IMPLEMENT_MAIN([] {
  func_1();
  eprintln("This line should not be executed.");
});
