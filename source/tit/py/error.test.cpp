/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "tit/py/error.hpp"
#include "tit/py/mapping.hpp"
#include "tit/py/number.hpp"

#include "tit/testing/test.hpp"

namespace tit {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("Traceback") {
  /// @todo Add tests.
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("BaseException") {
  /// @todo Add tests.
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("py::is_error_set / py::clear_error") {
  REQUIRE_FALSE(py::is_error_set());
  SUBCASE("raise and catch") {
    try {
      py::raise_type_error("some message");
    } catch (const py::ErrorException& e) {
      REQUIRE_FALSE(py::is_error_set());
    }
    CHECK_FALSE(py::is_error_set());
  }
  SUBCASE("raise, catch and restore") {
    try {
      py::raise_type_error("some message");
    } catch (py::ErrorException& e) {
      REQUIRE_FALSE(py::is_error_set());
      e.restore();
    }
    REQUIRE(py::is_error_set());
    py::clear_error();
    CHECK_FALSE(py::is_error_set());
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("py::expect") {
  SUBCASE("success") {
    const auto obj = py::Int{1};
    py::expect<py::Int>(obj);
  }
  SUBCASE("failure") {
    SUBCASE("concrete type") {
      CHECK_THROWS_MSG(py::expect<py::Float>(py::Dict{}),
                       py::ErrorException,
                       "TypeError: expected 'float', got 'dict'");
    }
    SUBCASE("abstract type") {
      CHECK_THROWS_MSG(py::expect<py::Mapping>(py::Int{}),
                       py::ErrorException,
                       "TypeError: expected 'Mapping', got 'int'");
    }
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
