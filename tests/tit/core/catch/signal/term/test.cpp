/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <csignal>
#include <cstdlib>
#include <iostream>
#include <print>

#include "tit/core/main.hpp"

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

namespace tit {
namespace {

[[gnu::noinline]] void func_3() {
  std::println(std::cerr, "func_3");
  std::println(std::cerr, "Sending SIGTERM...");
  static_cast<void>(std::raise(SIGTERM));
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
  static_cast<void>(std::atexit([] { std::println(std::cerr, "At exit..."); }));
  func_1();
  std::println(std::cerr, "This line should not be executed.");
});
