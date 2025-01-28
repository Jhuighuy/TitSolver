/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "tit/py/error.hpp"
#include "tit/py/mapping.hpp"
#include "tit/py/module.hpp"
#include "tit/py/number.hpp"
#include "tit/py/sequence.hpp"

#include "tit/testing/test.hpp"

namespace tit {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("py::Module") {
  SUBCASE("typing") {
    CHECK(py::Module::type().fully_qualified_name() == "module");
    CHECK(py::Module::isinstance(py::import_("numpy")));
    CHECK_FALSE(py::Module::isinstance(py::Int{}));
  }
  SUBCASE("properties") {
    const auto module = py::import_("numpy");
    CHECK(module.name() == "numpy");
    CHECK(module.dict().has_key("ndarray"));
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("py::import_") {
  SUBCASE("existing module") {
    py::import_("numpy");
  }
  SUBCASE("non-existing module") {
    CHECK_THROWS_MSG(py::import_("does_not_exist"),
                     py::ErrorException,
                     "ModuleNotFoundError: No module named 'does_not_exist'");
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("py::module_") {
  const auto module = py::module_("test_module");
  CHECK(module.dict().valid());
  REQUIRE(module.dict().has_key("__name__"));
  CHECK(module.dict()["__name__"] == py::Str{"test_module"});
  CHECK(module.name() == "test_module");
  // `add` and `def` are tested in the integration test.
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
