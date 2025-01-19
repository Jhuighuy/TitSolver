/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <string>
#include <string_view>

#include "tit/core/basic_types.hpp"

#include "tit/py/cast.hpp"
#include "tit/py/error.hpp"
#include "tit/py/number.hpp"
#include "tit/py/object.hpp"
#include "tit/py/sequence.hpp"

#include "tit/testing/test.hpp"

namespace tit {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("py::impl::Converter<Bool>") {
  SUBCASE("object") {
    CHECK(py::object(true) == py::Bool{true});
    CHECK(py::object(false) == py::Bool{false});
  }
  SUBCASE("extract") {
    SUBCASE("success") {
      CHECK(py::extract<bool>(py::Bool{true}));
      CHECK_FALSE(py::extract<bool>(py::Bool{false}));
    }
    SUBCASE("failure") {
      CHECK_THROWS_MSG(py::extract<bool>(py::make_list(1, 2, 3)),
                       py::ErrorException,
                       "TypeError: expected 'bool', got 'list'");
    }
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("py::impl::Converter<Int>") {
  SUBCASE("object") {
    CHECK(py::object(int8_t{1}) == py::Int{1});
    CHECK(py::object(uint16_t{2}) == py::Int{2});
    CHECK(py::object(int64_t{3}) == py::Int{3});
  }
  SUBCASE("extract") {
    SUBCASE("success") {
      CHECK(py::extract<uint8_t>(py::Int{1}) == uint8_t{1});
      CHECK(py::extract<int16_t>(py::Int{2}) == int16_t{2});
      CHECK(py::extract<uint64_t>(py::Int{3}) == uint64_t{3});
    }
    SUBCASE("failure") {
      CHECK_THROWS_MSG(py::extract<int>(py::make_list(1, 2, 3)),
                       py::ErrorException,
                       "TypeError: expected 'int', got 'list'");
    }
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("py::impl::Converter<Float>") {
  using long_double_t = long double;
  SUBCASE("object") {
    CHECK(py::object(1.0F) == py::Float{1.0});
    CHECK(py::object(2.0) == py::Float{2.0});
    CHECK(py::object(long_double_t{3.0}) == py::Float{3.0});
  }
  SUBCASE("extract") {
    SUBCASE("success") {
      CHECK(py::extract<float>(py::Float{1.0}) == 1.0F);
      CHECK(py::extract<double>(py::Float{2.0}) == 2.0);
      CHECK(py::extract<long_double_t>(py::Float{3.0}) == long_double_t{3.0});
    }
    SUBCASE("failure") {
      CHECK_THROWS_MSG(py::extract<double>(py::make_list(1, 2, 3)),
                       py::ErrorException,
                       "TypeError: expected 'float', got 'list'");
    }
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("py::impl::Converter<Str>") {
  SUBCASE("object") {
    CHECK(py::object("abc") == py::Str{"abc"});
    CHECK(py::object(CStrView{"abc"}) == py::Str{"abc"});
    CHECK(py::object(std::string{"abc"}) == py::Str{"abc"});
    CHECK(py::object(std::string_view{"abc"}) == py::Str{"abc"});
  }
  SUBCASE("extract") {
    SUBCASE("success") {
      CHECK(py::extract<CStrView>(py::Str{"abc"}) == "abc");
      CHECK(py::extract<std::string>(py::Str{"abc"}) == "abc");
      CHECK(py::extract<std::string_view>(py::Str{"abc"}) == "abc");
    }
    SUBCASE("failure") {
      CHECK_THROWS_MSG(py::extract<std::string_view>(py::make_list(1, 2, 3)),
                       py::ErrorException,
                       "TypeError: expected 'str', got 'list'");
    }
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("py::impl::Converter<Object>") {
  const py::Object obj = py::Int{1};
  SUBCASE("object") {
    CHECK(py::object(obj).is(obj));
  }
  SUBCASE("extract") {
    SUBCASE("success") {
      CHECK(py::extract<py::Int>(py::object(obj)).is(obj));
    }
    SUBCASE("failure") {
      CHECK_THROWS_MSG(py::extract<py::Bool>(obj),
                       py::ErrorException,
                       "TypeError: expected 'bool', got 'int'");
    }
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
