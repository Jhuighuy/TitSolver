/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "tit/py/cast.hpp"
#include "tit/py/error.hpp"
#include "tit/py/module.hpp"
#include "tit/py/number.hpp"
#include "tit/py/object.hpp"
#include "tit/py/sequence.hpp"
#include "tit/py/type.hpp"

#include "tit/py/interpreter.testing.hpp"
#include "tit/testing/test.hpp"

namespace tit {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("py::Type") {
  SUBCASE("typing") {
    CHECK(py::Type::type().fully_qualified_name() == "type");
    CHECK(py::Type::isinstance(py::type(py::Int{})));
    CHECK_FALSE(py::Type::isinstance(py::Int{}));
  }
  SUBCASE("properties") {
    SUBCASE("builtin types") {
      const auto int_type = py::type(py::Int{});
      CHECK(int_type.name() == "int");
      CHECK(int_type.qualified_name() == "int");
      CHECK(int_type.fully_qualified_name() == "int");
      CHECK(int_type.module_name() == "builtins");
    }
    SUBCASE("third-party types") {
      const auto numpy = py::import_("numpy");
      const auto ndarray = py::cast<py::Type>(numpy.attr("ndarray"));
      CHECK(ndarray.name() == "ndarray");
      CHECK(ndarray.qualified_name() == "ndarray");
      CHECK(ndarray.fully_qualified_name() == "numpy.ndarray");
      CHECK(ndarray.module_name() == "numpy");
    }
  }
  SUBCASE("methods") {
    SUBCASE("is_subtype_of") {
      CHECK(py::BaseException::type().is_subtype_of(py::BaseException::type()));
      const auto SystemError =
          py::cast<py::Type>(testing::interpreter().eval("SystemError"));
      CHECK(SystemError.is_subtype_of(py::BaseException::type()));
      CHECK_FALSE(SystemError.is_subtype_of(py::Int::type()));
    }
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("py::type") {
  const py::Object obj = py::Int{1};
  CHECK(py::type(obj).is(py::Int::type()));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("py::type_name") {
  SUBCASE("concrete type") {
    CHECK(py::type_name<py::Int>() == "int");
    CHECK(py::type_name<py::Float>() == "float");
  }
  SUBCASE("abstract type") {
    CHECK(py::type_name<py::Sequence>() == "Sequence");
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
