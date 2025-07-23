/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// Here I am using <iostream> instead of our routines to avoid the weird
// segfault from the `backtrace` function in the signal handler.
#include <iostream>

#include "tit/main/main.hpp"

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

namespace {

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

} // namespace

void tit_main(CmdArgs /*args*/) {
  func_1();
  std::cerr << "This line should not be executed.\n";
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
