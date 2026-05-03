/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <chrono>
#include <iostream>
#include <print>
#include <thread>

#include "tit/core/main.hpp"

namespace tit {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

[[gnu::noinline]] void func_3() {
  std::println(std::cerr, "func_3");
  std::println(std::cerr, "Creating a joinable thread...");
  std::thread(
      [] { std::this_thread::sleep_for(std::chrono::milliseconds(100)); });
}

[[gnu::noinline]] void func_2() {
  std::println(std::cerr, "func_2");
  func_3();
}

[[gnu::noinline]] void func_1() {
  std::println(std::cerr, "func_1");
  func_2();
}

} // namespace
} // namespace tit

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TIT_IMPLEMENT_MAIN([] {
  func_1();
  std::println(std::cerr, "This line should not be executed.");
});
