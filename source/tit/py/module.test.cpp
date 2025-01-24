/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "tit/py/error.hpp"
#include "tit/py/mapping.hpp"
#include "tit/py/module.hpp"
#include "tit/py/number.hpp"

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
  SUBCASE("import") {
    SUBCASE("existing module") {
      py::import_("numpy");
    }
    SUBCASE("non-existing module") {
      CHECK_THROWS_MSG(py::import_("does_not_exist"),
                       py::ErrorException,
                       "ModuleNotFoundError: No module named 'does_not_exist'");
    }
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
