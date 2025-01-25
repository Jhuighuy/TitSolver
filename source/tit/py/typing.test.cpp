/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "tit/py/number.hpp"
#include "tit/py/sequence.hpp"
#include "tit/py/typing.hpp"

#include "tit/testing/test.hpp"

namespace tit {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("py::Union") {
  SUBCASE("typing") {
    CHECK(py::Union<py::Int, py::Float>::type_name() == "int | float");
    CHECK(py::Union<py::Int, py::Float>::isinstance(py::Int{}));
    CHECK(py::Union<py::Int, py::Float>::isinstance(py::Float{}));
    CHECK_FALSE(py::Union<py::Int, py::Float>::isinstance(py::Str{}));
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
