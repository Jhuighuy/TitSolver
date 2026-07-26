/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "tit/core/exception.hpp"
#include "tit/core/math.hpp"
#include "tit/core/poly.hpp"
#include "tit/core/rational.hpp"
#include "tit/testing/test.hpp"

namespace tit {
namespace {

// Note: expressions given to the test macros carry an extra layer of
// parentheses, since the preprocessor does not treat braces as grouping.

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("Monomial") {
  SUBCASE("unit") {
    const Monomial mono;
    CHECK(mono.is_unit());
    CHECK(mono.power("x") == 0);
    CHECK(mono.powers().empty());
  }
  SUBCASE("var") {
    const auto mono = Monomial::var("y");
    CHECK_FALSE(mono.is_unit());
    CHECK(mono.power("x") == 0);
    CHECK(mono.power("y") == 1);
  }
  SUBCASE("with_power") {
    const auto mono = Monomial::var("y").with_power("z", 3);
    CHECK(mono.power("y") == 1);
    CHECK(mono.power("z") == 3);
  }
  SUBCASE("zero powers are not stored") {
    const auto mono = Monomial::var("y").with_power("y", 0);
    CHECK(mono.is_unit());
    CHECK((mono == Monomial{}));
  }
  SUBCASE("operator*") {
    const auto mono = Monomial::var("x") * Monomial::var("x") * //
                      Monomial::var("z");
    CHECK(mono.power("x") == 2);
    CHECK(mono.power("y") == 0);
    CHECK(mono.power("z") == 1);
  }
  SUBCASE("empty name") {
    CHECK_THROWS_MSG(Monomial::var(""), Exception, "must not be empty");
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("Poly") {
  SUBCASE("zero") {
    const Poly p;
    CHECK(p.is_zero());
    CHECK(p.is_constant());
    CHECK(p.terms().empty());
    CHECK(p.as_rational() == 0);
  }
  SUBCASE("constant") {
    const Poly p{Rational{3, 4}};
    CHECK_FALSE(p.is_zero());
    CHECK(p.is_constant());
    CHECK((p.as_rational() == Rational{3, 4}));
  }
  SUBCASE("integer constant") {
    const Poly p = 5;
    CHECK(p.as_rational() == 5);
  }
  SUBCASE("var") {
    const auto x = Poly::var("x");
    CHECK_FALSE(x.is_zero());
    CHECK_FALSE(x.is_constant());
    CHECK(x.terms().size() == 1);
    CHECK_THROWS_MSG(x.as_rational(), Exception, "not a constant");
  }
  SUBCASE("zero coefficients are not stored") {
    const auto x = Poly::var("x");
    CHECK((x - x).terms().empty());
    CHECK((x * 0).terms().empty());
  }
}

TEST_CASE("Poly::degree") {
  const auto x = Poly::var("x");
  const auto y = Poly::var("y");
  const auto p = pow(x, 3) * y + x + 1;
  CHECK(p.degree("x") == 3);
  CHECK(p.degree("y") == 1);
  CHECK(p.degree("z") == 0);
  CHECK(Poly{}.degree("x") == 0);
}

TEST_CASE("Poly::uses") {
  const auto x = Poly::var("x");
  const auto y = Poly::var("y");
  const auto p = x * y - y;
  CHECK(p.uses("x"));
  CHECK(p.uses("y"));
  CHECK_FALSE(p.uses("z"));
}

TEST_CASE("Poly::coeff") {
  const auto x = Poly::var("x");
  const auto y = Poly::var("y");
  // `p = x^2 * y + 2 * x + 3`.
  const auto p = pow(x, 2) * y + 2 * x + 3;
  CHECK(p.coeff("x", 0) == 3);
  CHECK(p.coeff("x", 1) == 2);
  CHECK(p.coeff("x", 2) == y);
  CHECK(p.coeff("x", 3).is_zero());
  CHECK(p.coeff("y", 1) == pow(x, 2));
}

TEST_CASE("Poly::common_factor") {
  const auto x = Poly::var("x");
  const auto y = Poly::var("y");
  SUBCASE("none") {
    CHECK((x + y).common_factor().is_unit());
    CHECK((x + 1).common_factor().is_unit());
  }
  SUBCASE("single variable") {
    const auto mono = (pow(x, 3) + pow(x, 2)).common_factor();
    CHECK(mono.power("x") == 2);
  }
  SUBCASE("several variables") {
    const auto mono = (pow(x, 2) * y + x * pow(y, 3)).common_factor();
    CHECK(mono.power("x") == 1);
    CHECK(mono.power("y") == 1);
  }
  SUBCASE("zero polynomial") {
    CHECK(Poly{}.common_factor().is_unit());
  }
}

TEST_CASE("Poly::divide") {
  const auto x = Poly::var("x");
  const auto y = Poly::var("y");
  SUBCASE("by the common factor") {
    const auto p = pow(x, 2) * y + x * pow(y, 3);
    CHECK(p.divide(p.common_factor()) == x + pow(y, 2));
  }
  SUBCASE("by the unit monomial") {
    CHECK((x + y).divide(Monomial{}) == x + y);
  }
  SUBCASE("not a common factor") {
    CHECK_THROWS_MSG((x + y).divide(Monomial::var("x")),
                     Exception,
                     "not a common factor");
  }
}

TEST_CASE("Poly::substitute") {
  const auto x = Poly::var("x");
  const auto y = Poly::var("y");
  SUBCASE("variable") {
    CHECK(pow(x + 1, 2).substitute("x", y) == pow(y + 1, 2));
  }
  SUBCASE("shift") {
    // Expanding about `x = -2` must give `(x + 2)^2`.
    CHECK(pow(x + 1, 2).substitute("x", x + 1) == pow(x + 2, 2));
  }
  SUBCASE("constant") {
    const auto p = pow(x + 1, 2).substitute("x", Poly{Rational{1, 2}});
    CHECK((p.as_rational() == Rational{9, 4}));
  }
}

TEST_CASE("Poly::reduce_square") {
  const auto x = Poly::var("x");
  const auto y = Poly::var("y");
  const auto z = Poly::var("z");
  SUBCASE("even power") {
    CHECK(pow(x, 4).reduce_square("x", y + z) == pow(y + z, 2));
  }
  SUBCASE("odd power") {
    CHECK(pow(x, 5).reduce_square("x", y + z) == pow(y + z, 2) * x);
  }
  SUBCASE("low powers are left alone") {
    CHECK((x + 1).reduce_square("x", y) == x + 1);
  }
  SUBCASE("mixed") {
    // `x` stands for `sqrt(y)`, so `y * x^3 + x^2` becomes `y^2 * x + y`.
    const auto p = y * pow(x, 3) + pow(x, 2);
    CHECK(p.reduce_square("x", y) == pow(y, 2) * x + y);
  }
}

TEST_CASE("Poly::operator+") {
  const auto x = Poly::var("x");
  const auto y = Poly::var("y");
  CHECK(x + y == y + x);
  CHECK(x + 0 == x);
  CHECK((x + (-x) == Poly{}));
  SUBCASE("compound") {
    auto p = x;
    p += y;
    CHECK(p == x + y);
  }
  SUBCASE("compound with itself") {
    auto p = x + y;
    p += p;
    CHECK(p == 2 * (x + y));
  }
}

TEST_CASE("Poly::operator-") {
  const auto x = Poly::var("x");
  const auto y = Poly::var("y");
  CHECK(-(x - y) == y - x);
  CHECK(1 - x == -(x - 1));
}

TEST_CASE("Poly::operator*") {
  const auto x = Poly::var("x");
  const auto y = Poly::var("y");
  CHECK((x + y) * (x - y) == pow(x, 2) - pow(y, 2));
  CHECK(x * 1 == x);
  CHECK((x * Rational{1, 2} * 2 == x));
  CHECK((x * y).degree("x") == 1);
}

TEST_CASE("Poly::operator/") {
  const auto x = Poly::var("x");
  CHECK((2 * x + 4) / Rational{2} == x + 2);
  CHECK((x / Rational{1, 3} == 3 * x));
  CHECK_THROWS_MSG(x / Rational{0}, Exception, "division by zero");
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// `pow` itself lives in `math.hpp`; check that it works for the polynomials.
TEST_CASE("pow(Poly)") {
  const auto x = Poly::var("x");
  CHECK(pow(x, 0) == 1);
  CHECK(pow(x, 1) == x);
  CHECK(pow(x + 1, 3) == pow(x, 3) + 3 * pow(x, 2) + 3 * x + 1);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
