/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <filesystem>
#include <fstream>

#include "tit/core/exception.hpp"

#include "tit/py/core.hpp"

#include "tit/py/_embed/interpreter.testing.hpp"
#include "tit/testing/test.hpp"

namespace tit {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("py::Interpreter::eval") {
  SUBCASE("success") {
    CHECK(extract<int>(testing::interpreter().eval("1 + 2")) == 3);
    CHECK(extract<int>(testing::interpreter().eval(R"PY(
      1 + 2
    )PY")) == 3);
  }
  SUBCASE("failure") {
    CHECK_THROWS_MSG( //
        testing::interpreter().eval("'abc' - 1"),
        py::Error,
        "TypeError: unsupported operand type(s) for -: 'str' and 'int'");
    py::clear_error();
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("py::Interpreter::exec") {
  CHECK(testing::interpreter().exec("print('Hello, exec!')"));
  CHECK(testing::interpreter().exec(R"PY(
    print('Hello,')
    print('multiline exec!')
  )PY"));
  CHECK_FALSE(testing::interpreter().exec("print('abc' - 1)"));
}

TEST_CASE("py::Interpreter::exec_file") {
  const std::filesystem::path file_name{"test.py"};
  if (std::filesystem::exists(file_name)) {
    REQUIRE(std::filesystem::remove(file_name));
  }
  SUBCASE("file exists") {
    SUBCASE("success") {
      {
        std::ofstream file{file_name};
        file << "print('Hello, file!')\n";
      }
      CHECK(testing::interpreter().exec_file(file_name.string()));
    }
    SUBCASE("failure") {
      {
        std::ofstream file{file_name};
        file << "import does_not_exist\n";
      }
      CHECK_FALSE(testing::interpreter().exec_file(file_name.string()));
    }
  }
  SUBCASE("cannot open file") {
    CHECK_THROWS_MSG(testing::interpreter().exec_file(file_name.string()),
                     Exception,
                     "Failed to open file 'test.py'.");
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit