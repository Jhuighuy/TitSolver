/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <iostream>
#include <string_view>

#include "tit/py/bind.hpp"
#include "tit/py/core.hpp"

namespace tit {
namespace {

struct TestClass {};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void bind_test_module(py::cpp::Module& m) {
  m.def<"hello", []() { return "Hello, world!"; }>();
  m.def<"echo",
        [](std::string_view arg) { return arg; },
        py::cpp::Param<std::string_view, "arg">>();
  m.def<"echo_kwargs",
        [](int a, int b, int p, int q) { return p + q + a + b; },
        py::cpp::Param<int, "p">,
        py::cpp::Param<int, "q">,
        py::cpp::Param<int, "a", 1>,
        py::cpp::Param<int, "b", 2>>();

  const auto c = py::cpp::class_<"TestClass", TestClass>(m);
  c.def<"__init__",
        [](TestClass& /*self*/, int a) {
          std::cout << "__init__ " << a << std::endl; // NOLINT
          return py::None();
        },
        py::cpp::Param<int, "a">>();
  c.def<"hello_w", [](const TestClass& self) {
    return py::make_tuple(py::Str{"Hello, class!"}, py::cpp::find(self));
  }>();
  c.prop<"a",
         [](const TestClass& /*self*/) { return py::Int{14}; },
         [](const TestClass& /*self*/, const py::Object& value) {
           std::cout << "set a = " << py::extract<int>(value)
                     << std::endl; // NOLINT
         }>();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit

TIT_PYCPP_MODULE(test_module, tit::bind_test_module)
