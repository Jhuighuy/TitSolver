/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <csignal>
#include <cstdlib>

#include "tit/core/main.hpp"
#include "tit/core/print.hpp"

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

namespace tit {
namespace {

[[gnu::noinline]] void func_3() {
  eprintln("func_3");
  eprintln("Sending SIGTERM...");
  static_cast<void>(std::raise(SIGTERM));
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
  static_cast<void>(std::atexit([] { eprintln("At exit..."); }));
  func_1();
  eprintln("This line should not be executed.");
});
