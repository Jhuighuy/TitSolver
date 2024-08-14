/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <numbers>
#include <utility>

#include "tit/core/math.hpp"

#include "tit/testing/func_utils.hpp"
#include "tit/testing/test.hpp"

namespace tit {
namespace {

#define NUM_TYPES float, double

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE_TEMPLATE("newton", Num, NUM_TYPES) {
  SUBCASE("quadratic") {
    // Ensure the solver works for basic functions.
    SUBCASE("success") {
      constexpr auto f = [](Num x) {
        testing::prevent_constexpr();
        return std::pair{pow2(x) - Num{4.0}, Num{2.0} * x};
      };
      const auto result = newton(Num{1.0}, f);
      REQUIRE(result);
      constexpr auto root = Num{2.0};
      CHECK(approx_equal_to(result.value(), root));
    }

    // Ensure the solver fails after the iteration limit is exceeded if no
    // actual root can be found.
    SUBCASE("not converged") {
      constexpr auto f = [](Num x) {
        testing::prevent_constexpr();
        return std::pair{pow2(x) + Num{4.0}, Num{2.0} * x};
      };
      const auto result = newton(Num{1.0}, f, /*eps=*/1.0e-16, /*max_iter=*/2);
      REQUIRE(!result);
      CHECK(result.error() == NewtonError::not_converged);
    }
  }

  SUBCASE("cubic") {
    // Ensure the solver fails if the zero derivative was reached during  the
    // computation.
    SUBCASE("zero derivative") {
      constexpr auto f = [](Num x) {
        testing::prevent_constexpr();
        return std::pair{pow3(x) - Num{12.0} * x + Num{2.0},
                         Num{3.0} * pow2(x) - Num{12.0}};
      };
      const auto result = newton(Num{2.0}, f);
      REQUIRE(!result);
      CHECK(result.error() == NewtonError::zero_deriv);
    }
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE_TEMPLATE("bisection", Num, NUM_TYPES) {
  SUBCASE("quadratic") {
    constexpr Num root{2.0};
    constexpr auto f = [](Num x) {
      testing::prevent_constexpr();
      return pow2(x) - pow2(root);
    };

    // Ensure the solver works for basic functions.
    SUBCASE("success") {
      const auto result = bisection(Num{1.5}, Num{3.5}, f);
      REQUIRE(result);
      CHECK(approx_equal_to(result.value(), root));
    }

    // Ensure the solver completes with a single function evaluation if the root
    // is already located on the left side of the search interval.
    SUBCASE("early min") {
      CountedFunc counted_f{f};
      const auto result = bisection(Num{2.0}, Num{4.0}, counted_f);
      REQUIRE(result);
      CHECK(approx_equal_to(result.value(), root));
      CHECK(counted_f.count() == 1);
    }

    // Ensure the solver completes with two function evaluations if the root is
    // already located on the right side of the search interval.
    SUBCASE("early max") {
      CountedFunc counted_f{f};
      const auto result = bisection(Num{0.0}, Num{2.0}, counted_f);
      REQUIRE(result);
      CHECK(approx_equal_to(result.value(), root));
      CHECK(counted_f.count() == 2);
    }

    // Ensure the solver terminates if function values on the ends of the search
    // interval have same signs.
    SUBCASE("sign error") {
      const auto result = bisection(Num{2.5}, Num{5.5}, f);
      REQUIRE(!result);
      CHECK(result.error() == BisectionError::sign);
    }
  }

  SUBCASE("sin") {
    // Ensure the solver works for a bit more complex functions.
    SUBCASE("success") {
      constexpr auto f = [](Num x) {
        testing::prevent_constexpr();
        return sin(x) + Num{0.5};
      };
      const auto result = bisection(Num{1.0}, Num{4.0}, f);
      REQUIRE(result);
      const Num root{7.0 * std::numbers::pi / 6.0};
      CHECK(approx_equal_to(result.value(), root));
    }

    // Ensure the solver fails after the iteration limit is exceeded if no
    // actual root can be found.
    SUBCASE("not converged") {
      const auto f = [](Num x) {
        testing::prevent_constexpr();
        return sin(x) - inverse(x);
      };
      const auto result = bisection(Num{0.1},
                                    Num{1.2},
                                    f,
                                    /*eps=*/1.0e-16,
                                    /*max_iter=*/2);
      REQUIRE(!result);
      CHECK(result.error() == BisectionError::not_converged);
    }
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
