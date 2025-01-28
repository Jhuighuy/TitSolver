/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <format>
#include <stdexcept>

#include "tit/py/func.hpp"
#include "tit/py/module.hpp"

namespace tit {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void bind_test_module(py::Module& m) {
  m.add("PI", 3.14);
  m.def<"hello", []() { return "Hello, world!"; }>();
  m.def<"test_func",
        [](int p, int q, int a, int b) {
          return std::format("p={} q={} a={} b={}", p, q, a, b);
        },
        py::Param<int, "p">,
        py::Param<int, "q">,
        py::Param<int, "a", 1>,
        py::Param<int, "b", 2>>();
  m.def<"throw", []() { throw std::logic_error{"Exception from C++!"}; }>();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit

TIT_PYTHON_MODULE(test_module, tit::bind_test_module)
