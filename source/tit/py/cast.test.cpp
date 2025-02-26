/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <string>
#include <string_view>

#include "tit/core/basic_types.hpp"

#include "tit/py/cast.hpp"
#include "tit/py/error.hpp"
#include "tit/py/mapping.hpp"
#include "tit/py/number.hpp"
#include "tit/py/object.hpp"
#include "tit/py/sequence.hpp"

#include "tit/testing/test.hpp"

namespace tit {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("py::cast<object-type>") {
  SUBCASE("success") {
    const auto obj = py::Int{1};
    py::cast<py::Int>(obj);
  }
  SUBCASE("failure") {
    SUBCASE("concrete type") {
      CHECK_THROWS_MSG(py::cast<py::Float>(py::Dict{}),
                       py::ErrorException,
                       "TypeError: expected 'float', got 'dict'");
    }
    SUBCASE("abstract type") {
      CHECK_THROWS_MSG(py::cast<py::Mapping>(py::Int{}),
                       py::ErrorException,
                       "TypeError: expected 'Mapping', got 'int'");
    }
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("py::cast<bool>") {
  SUBCASE("to object") {
    CHECK(py::cast<py::Object>(true) == py::Bool{true});
    CHECK(py::cast<py::Object>(false) == py::Bool{false});
  }
  SUBCASE("from object") {
    SUBCASE("success") {
      CHECK(py::cast<bool>(py::Bool{true}));
      CHECK_FALSE(py::cast<bool>(py::Bool{false}));
    }
    SUBCASE("failure") {
      CHECK_THROWS_MSG(py::cast<bool>(py::make_list(1, 2, 3)),
                       py::ErrorException,
                       "TypeError: expected 'bool', got 'list'");
    }
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("py::cast<int-type>") {
  SUBCASE("to object") {
    CHECK(py::cast<py::Object>(int8_t{1}) == py::Int{1});
    CHECK(py::cast<py::Object>(uint16_t{2}) == py::Int{2});
    CHECK(py::cast<py::Object>(int64_t{3}) == py::Int{3});
  }
  SUBCASE("from object") {
    SUBCASE("success") {
      CHECK(py::cast<uint8_t>(py::Int{1}) == uint8_t{1});
      CHECK(py::cast<int16_t>(py::Int{2}) == int16_t{2});
      CHECK(py::cast<uint64_t>(py::Int{3}) == uint64_t{3});
    }
    SUBCASE("failure") {
      CHECK_THROWS_MSG(py::cast<int>(py::make_list(1, 2, 3)),
                       py::ErrorException,
                       "TypeError: expected 'int', got 'list'");
    }
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("py::cast<float-type>") {
  using long_double_t = long double;
  SUBCASE("to object") {
    CHECK(py::cast<py::Object>(1.0F) == py::Float{1.0});
    CHECK(py::cast<py::Object>(2.0) == py::Float{2.0});
    CHECK(py::cast<py::Object>(long_double_t{3.0}) == py::Float{3.0});
  }
  SUBCASE("from object") {
    SUBCASE("success") {
      CHECK(py::cast<float>(py::Float{1.0}) == 1.0F);
      CHECK(py::cast<double>(py::Float{2.0}) == 2.0);
      CHECK(py::cast<long_double_t>(py::Float{3.0}) == long_double_t{3.0});
    }
    SUBCASE("failure") {
      CHECK_THROWS_MSG(py::cast<double>(py::make_list(1, 2, 3)),
                       py::ErrorException,
                       "TypeError: expected 'float', got 'list'");
    }
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("py::cast<Str>") {
  SUBCASE("to object") {
    CHECK(py::cast<py::Object>("abc") == py::Str{"abc"});
    CHECK(py::cast<py::Object>(CStrView{"abc"}) == py::Str{"abc"});
    CHECK(py::cast<py::Object>(std::string{"abc"}) == py::Str{"abc"});
    CHECK(py::cast<py::Object>(std::string_view{"abc"}) == py::Str{"abc"});
  }
  SUBCASE("from object") {
    SUBCASE("success") {
      CHECK(py::cast<CStrView>(py::Str{"abc"}) == "abc");
      CHECK(py::cast<std::string>(py::Str{"abc"}) == "abc");
      CHECK(py::cast<std::string_view>(py::Str{"abc"}) == "abc");
    }
    SUBCASE("failure") {
      CHECK_THROWS_MSG(py::cast<std::string_view>(py::make_list(1, 2, 3)),
                       py::ErrorException,
                       "TypeError: expected 'str', got 'list'");
    }
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("py::cast<Object>") {
  const py::Object obj = py::Int{1};
  SUBCASE("to object") {
    CHECK(py::cast<py::Object>(obj).is(obj));
  }
  SUBCASE("from object") {
    SUBCASE("success") {
      CHECK(py::cast<py::Int>(py::Object(obj)).is(obj));
    }
    SUBCASE("failure") {
      CHECK_THROWS_MSG(py::cast<py::Bool>(obj),
                       py::ErrorException,
                       "TypeError: expected 'bool', got 'int'");
    }
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
