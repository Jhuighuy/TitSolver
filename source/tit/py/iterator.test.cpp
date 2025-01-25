/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "tit/py/error.hpp"
#include "tit/py/iterator.hpp"
#include "tit/py/number.hpp"
#include "tit/py/object.hpp"
#include "tit/py/sequence.hpp"

#include "tit/testing/test.hpp"

namespace tit {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("py::Iterator") {
  SUBCASE("typing") {
    CHECK(py::Iterator::type_name() == "iterator");
    CHECK(py::Iterator::isinstance(py::iter(py::List{})));
    CHECK_FALSE(py::Iterator::isinstance(py::Int{}));
  }
  SUBCASE("construction") {
    SUBCASE("from iterable") {
      const auto iterable = py::make_list(1, 2, 3);
      const auto iter = py::iter(iterable);
      CHECK(iter.next() == py::Int{1});
      CHECK(iter.next() == py::Int{2});
      CHECK(iter.next() == py::Int{3});
      CHECK_FALSE(iter.next());
    }
    SUBCASE("failure") {
      CHECK_THROWS_MSG(py::iter(py::None()),
                       py::ErrorException,
                       "TypeError: 'NoneType' object is not iterable");
    }
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
