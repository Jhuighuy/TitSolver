/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "tit/core/exception.hpp"
#include "tit/sym/expression.hpp"
#include "tit/sym/polynomial.hpp"
#include "tit/testing/test.hpp"

namespace tit::sym {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("sym::as_polynomial") {
  const Context context;
  const auto x = context.symbol("x");
  const auto y = context.symbol("y");
  const auto polynomial = as_polynomial((x + 1) * (x + 2) + y * pow(x, 3), x);

  REQUIRE(polynomial.degree() == 3);
  CHECK(polynomial[0] == context.integer(2));
  CHECK(polynomial[1] == context.integer(3));
  CHECK(polynomial[2] == context.integer(1));
  CHECK(polynomial[3] == y);
}

TEST_CASE("sym::as_polynomial rejects non-polynomials") {
  const Context context;
  const auto x = context.symbol("x");
  const auto n = context.symbol("n");

  CHECK_THROWS_MSG(as_polynomial(1 / x, x), Exception, "negative power");
  CHECK_THROWS_MSG(as_polynomial(pow(x, n), x), Exception, "not polynomial");
}

TEST_CASE("sym::horner") {
  const Context context;
  const auto x = context.symbol("x");
  const auto y = context.symbol("y");

  CHECK(horner(pow(x + 1, 3), {x}) == 1 + x * (3 + x * (3 + x)));
  CHECK(horner(x * y + x + y + 1, {x, y}) == 1 + y + x * (1 + y));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit::sym
