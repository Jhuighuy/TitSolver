/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <concepts>
#include <limits>
#include <numbers>
#include <tuple>

#include "tit/core/math.hpp"

#include "tit/testing/func_utils.hpp"
#include "tit/testing/test.hpp"

namespace tit {
namespace {

#define NUM_TYPES float, double

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE_TEMPLATE("sign", Num, NUM_TYPES) {
  CHECK(sign(Num{0}) == Num{0});
  CHECK(sign(+Num{2}) == +Num{1});
  CHECK(sign(-Num{2}) == -Num{1});
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE_TEMPLATE("pow", Num, NUM_TYPES) {
  CHECK(pow2(+Num{2}) == +Num{4});
  CHECK(pow2(-Num{2}) == +Num{4});
  CHECK(pow3(+Num{2}) == +Num{8});
  CHECK(pow3(-Num{2}) == -Num{8});
  CHECK(pow4(+Num{2}) == +Num{16});
  CHECK(pow4(-Num{2}) == +Num{16});
  CHECK(pow5(+Num{2}) == +Num{32});
  CHECK(pow5(-Num{2}) == -Num{32});
  CHECK(pow6(+Num{2}) == +Num{64});
  CHECK(pow6(-Num{2}) == +Num{64});
  CHECK(pow7(+Num{2}) == +Num{128});
  CHECK(pow7(-Num{2}) == -Num{128});
  CHECK(pow8(+Num{2}) == +Num{256});
  CHECK(pow8(-Num{2}) == +Num{256});
  CHECK(pow9(+Num{2}) == +Num{512});
  CHECK(pow9(-Num{2}) == -Num{512});
  if constexpr (std::floating_point<Num>) {
    CHECK(pow(+Num{2}, 10) == Num{1024});
    CHECK(pow(-Num{2}, 10) == Num{1024});
  }
}

TEST_CASE_TEMPLATE("horner", Num, NUM_TYPES) {
  CHECK(horner(Num{1}, {Num{1}}) == Num{1});
  CHECK(horner(Num{3}, {Num{1}, -Num{3}, Num{2}}) == Num{10});
  CHECK(horner(-Num{2}, {Num{4}, -Num{1}, Num{3}}) == Num{18});
  CHECK(horner(Num{3}, {Num{6}, Num{1}, -Num{4}, Num{1}}) == Num{0});
}

TEST_CASE_TEMPLATE("inverse", Num, NUM_TYPES) {
  CHECK(inverse(Num{2}) == 0.5);
  CHECK(inverse(Num{8}) == 0.125);
}

TEST_CASE_TEMPLATE("rsqrt", Num, NUM_TYPES) {
  CHECK(rsqrt(Num{0.25}) == Num{2.0});
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE_TEMPLATE("avg", Num, NUM_TYPES) {
  CHECK(avg(Num{1}, Num{2}) == static_cast<Num>(1.5));
  CHECK(avg(Num{1}, Num{2}, Num{3}) == static_cast<Num>(2.0));
}

TEST_CASE_TEMPLATE("gavg", Num, NUM_TYPES) {
  CHECK(havg(Num{1.0}, Num{4.0}) == Num{1.6});
}

TEST_CASE_TEMPLATE("gavg", Num, NUM_TYPES) {
  CHECK(gavg(Num{1.0}, Num{4.0}) == Num{2.0});
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE_TEMPLATE("tiny_number_v", Num, NUM_TYPES) {
  // Tiny number must be positive.
  STATIC_CHECK(tiny_number_v<Num> > 0.0);
  // Small number should be larger than machine epsilon.
  STATIC_CHECK(tiny_number_v<Num> >= std::numeric_limits<Num>::epsilon());
}

TEST_CASE_TEMPLATE("is_tiny", Num, NUM_TYPES) {
  // Check ordinary numbers.
  CHECK(is_tiny(+Num{0.0}));
  CHECK(is_tiny(-Num{0.0}));
  CHECK_FALSE(is_tiny(+Num{1.0}));
  CHECK_FALSE(is_tiny(-Num{1.0}));
  // Check if comparisons with `tiny_number_v` work as expected.
  CHECK(is_tiny(+tiny_number_v<Num>));
  CHECK(is_tiny(-tiny_number_v<Num>));
  CHECK(is_tiny(+Num{0.1} * tiny_number_v<Num>));
  CHECK(is_tiny(-Num{0.1} * tiny_number_v<Num>));
  CHECK_FALSE(is_tiny(+Num{2.0} * tiny_number_v<Num>));
  CHECK_FALSE(is_tiny(-Num{2.0} * tiny_number_v<Num>));
}

TEST_CASE_TEMPLATE("approx_equal_to", Num, NUM_TYPES) {
  // Check ordinary numbers.
  CHECK(approx_equal_to(Num{1.23}, Num{1.23}));
  CHECK(!approx_equal_to(Num{1.23}, Num{5.67}));
  // Check if comparisons with `tiny_number_v` work as expected.
  CHECK(approx_equal_to(Num{1.23}, Num{1.23} + tiny_number_v<Num>));
  CHECK(approx_equal_to(Num{1.23} - tiny_number_v<Num>, Num{1.23}));
  CHECK(approx_equal_to(Num{1.23}, Num{1.23} + Num{0.1} * tiny_number_v<Num>));
  CHECK(approx_equal_to(Num{1.23} - Num{0.1} * tiny_number_v<Num>, Num{1.23}));
  CHECK(!approx_equal_to(Num{1.23}, Num{1.23} + Num{2.0} * tiny_number_v<Num>));
  CHECK(!approx_equal_to(Num{1.23} - Num{2.0} * tiny_number_v<Num>, Num{1.23}));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE_TEMPLATE("newton_raphson", Num, NUM_TYPES) {
  using enum NewtonRaphsonStatus;
  SUBCASE("quadratic") {
    SUBCASE("success") {
      // Ensure the solver works for basic functions.
      auto x = Num{1.0};
      const auto f = [&x] {
        return std::tuple{pow2(x) - Num{4.0}, Num{2.0} * x};
      };
      constexpr auto root = Num{2.0};
      CHECK(newton_raphson(x, f) == success);
      CHECK(approx_equal_to(x, root));
    }
    SUBCASE("fail_max_iter") {
      // Ensure the solver fails after the iteration limit is exceeded
      // if no actual root can be found.
      auto x = Num{1.0};
      const auto f = [&x] {
        return std::tuple{pow2(x) + Num{4.0}, Num{2.0} * x};
      };
      CHECK(newton_raphson(x, f) == fail_max_iter);
    }
  }
  SUBCASE("cubic") {
    SUBCASE("failure_zero_derivative") {
      // Ensure the solver fails if the zero derivative was reached during
      // the computations.
      auto x = Num{2.0};
      const auto f = [&x] {
        return std::tuple{pow3(x) - Num{12.0} * x + Num{2.0},
                          Num{3.0} * pow2(x) - Num{12.0}};
      };
      CHECK(newton_raphson(x, f) == fail_zero_deriv);
    }
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE_TEMPLATE("bisection", Num, NUM_TYPES) {
  using enum BisectionStatus;
  SUBCASE("quadratic") {
    constexpr auto root = Num{2.0};
    const auto f = [](Num x) { return pow2(x) - pow2(root); };
    SUBCASE("success") {
      // Ensure the solver works for basic functions.
      Num min_x{1.5};
      Num max_x{3.5};
      CHECK(bisection(min_x, max_x, f) == success);
      CHECK(approx_equal_to(min_x, root));
      CHECK(approx_equal_to(max_x, root));
    }
    SUBCASE("success_early_min") {
      // Ensure the solver completes with a single function evaluation if the
      // root is already located on the left side of the search interval.
      Num min_x{2.0};
      Num max_x{4.0};
      CountedFunc counted_f{f};
      CHECK(bisection(min_x, max_x, counted_f) == success);
      CHECK(approx_equal_to(min_x, root));
      CHECK(approx_equal_to(max_x, root));
      CHECK(counted_f.count() == 1);
    }
    SUBCASE("success_early_max") {
      // Ensure the solver completes with two function evaluations if the root
      // is already located on the right side of the search interval.
      Num min_x{0.0};
      Num max_x{2.0};
      CountedFunc counted_f{f};
      CHECK(bisection(min_x, max_x, counted_f) == success);
      CHECK(approx_equal_to(min_x, root));
      CHECK(approx_equal_to(max_x, root));
      CHECK(counted_f.count() == 2);
    }
    SUBCASE("failure_sign") {
      // Ensure the solver terminates if function values on the ends of the
      // search interval have same signs.
      Num min_x{2.5};
      Num max_x{5.5};
      CHECK(bisection(min_x, max_x, f) == failure_sign);
    }
  }
  SUBCASE("sin") {
    SUBCASE("success") {
      // Ensure the solver works for a bit more complex functions.
      const auto f = [](Num x) { return sin(x) + Num{0.5}; };
      const Num root{7.0 * std::numbers::pi / 6.0};
      Num min_x{1.0};
      Num max_x{4.0};
      CHECK(bisection(min_x, max_x, f) == success);
      CHECK(approx_equal_to(min_x, root));
      CHECK(approx_equal_to(max_x, root));
    }
    SUBCASE("fail_max_iter") {
      // Ensure the solver fails after the iteration limit is exceeded
      // if no actual root can be found.
      const auto f = [](Num x) { return sin(x) - inverse(x); };
      Num min_x{0.1};
      Num max_x{1.2};
      CHECK(bisection(min_x, max_x, f) == fail_max_iter);
    }
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
