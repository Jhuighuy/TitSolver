/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <format>
#include <iostream>
#include <stdexcept>

#include "tit/py/class.hpp"
#include "tit/py/func.hpp"
#include "tit/py/module.hpp"
#include "tit/py/number.hpp"
#include "tit/py/object.hpp"
#include "tit/py/sequence.hpp"

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

  class TestClass {};
  const py::Class<TestClass> c{"TestClass", m};
  c.def<"__init__",
        [](TestClass& /*self*/, int a) {
          std::cout << "__init__ " << a << std::endl; // NOLINT
        },
        py::Param<int, "a">>();
  c.def<"hello_w", [](const TestClass& self) {
    return py::make_tuple(py::Str{"Hello, class!"}, py::find(self));
  }>();
  c.prop<"a",
         [](const TestClass& /*self*/) { return py::Int{14}; },
         [](const TestClass& /*self*/, const py::Object& value) {
           std::cout << "set a = " << py::extract<int>(value)
                     << std::endl; // NOLINT
         }>();

  m.add(c);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit

TIT_PYTHON_MODULE(test_module, tit::bind_test_module)
