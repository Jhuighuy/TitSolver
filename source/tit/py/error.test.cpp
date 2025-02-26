/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "tit/py/error.hpp"

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

} // namespace
} // namespace tit
