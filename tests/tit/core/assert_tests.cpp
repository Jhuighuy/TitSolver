/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <cstdlib>
#include <string>
#include <vector>

#include <fmt/core.h>

#include "tit/core/assert.hpp"
#include "tit/core/config.hpp"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-declarations"

namespace test_1 {

// Avoid using much compiler-specific stuff in this test, like lambdas and
// anonymous namespaced. Here I want just to check that basic backtrace parsing
// and syntax highlighting works.

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

namespace nested {

  // A function inside of a nested namespace.
  TIT_NO_INLINE void func_2(const std::vector<std::string>& args) {
    TIT_ENSURE(args.size() > 1, "Arguments should be specified!");
    // Do something, otherwise this function may be inlined!
    for (const auto& arg : args) {
      fmt::println("{}", arg);
    }
  }

} // namespace nested

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

// Just a normal function inside of a namespace.
// It's name (with template declaration is not too big, so it's not skipped).
template<class I>
TIT_NO_INLINE I func_1(I argc, char** argv) {
  std::vector<std::string> args(argv, argv + argc);
  nested::func_2(args);
  return 0;
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

} // namespace test_1

#pragma GCC diagnostic pop

int main(int argc, char** argv) {
  // Limit backtrace depth to two symbols to avoid in order to skip deep entries
  // with system calls.
  setenv("TIT_MAX_BACKTRACE", "3", true);
  return test_1::func_1(argc, argv);
}
